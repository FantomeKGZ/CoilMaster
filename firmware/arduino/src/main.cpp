#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#if defined(__AVR__)
#include <avr/io.h>
#include <avr/wdt.h>
#endif

#include "../../../Arduino/Config/CM_Features.h"
#include "../../../Arduino/Config/CM_Pins.h"

#include "../../../Core/CM_InputController.h"
#include "../../../Core/CM_StateMachine.h"
#include "../../../Core/CM_TurnSource.h"
#include "../../../Core/CM_UiModel.h"
#include "../../../Core/CM_WindingEvent.h"

#include "../../../Arduino/CM_BuzzerService.h"
#include "../../../Arduino/CM_DebouncedButton.h"
#include "../../../Arduino/CM_EepromPersistence.h"
#include "../../../Arduino/CM_HallCalibrationService.h"
#include "../../../Arduino/CM_HallTelemetry.h"
#include "../../../Arduino/CM_HallTurnSource.h"
#include "../../../Arduino/CM_HardwareSettings.h"
#include "../../../Arduino/CM_HardwareSettingsController.h"
#include "../../../Arduino/CM_Lcd1602View.h"
#include "../../../Arduino/CM_SsrController.h"
#include "../../../Arduino/CM_UartEventTransport.h"

#if CM_FEATURE_BOOT_DIAGNOSTICS
HardwareSerial& cmBootSerial = Serial;
#endif

#if !CM_FEATURE_DIAGNOSTICS
namespace
{
class NullDiagnosticSerial
{
public:
    void begin(unsigned long) {}
    void flush() {}
    void println() {}

    template <typename T>
    void print(const T&) {}

    template <typename T, typename U>
    void print(const T&, const U&) {}

    template <typename T>
    void println(const T&) {}
};

NullDiagnosticSerial diagnosticSerial;
}
#define Serial diagnosticSerial
#endif

#if defined(__AVR__)
// Capture reset provenance even in production builds where Serial diagnostics
// are compiled out. The LCD boot screen is the fail-safe field diagnostic.
uint8_t cmResetFlags __attribute__((section(".noinit")));
volatile uint8_t cmLastLoopPhase __attribute__((section(".noinit")));
volatile uint8_t cmLoopPhaseMagic __attribute__((section(".noinit")));

void cmCaptureResetFlags() __attribute__((naked, section(".init3")));
void cmCaptureResetFlags()
{
    // MCUSR only defines the low five reset-source bits on supported AVR
    // targets. Mask reserved bits so LCD evidence cannot report values such as
    // F4/FC that have no reset-source meaning.
    cmResetFlags = static_cast<uint8_t>(MCUSR & 0x1FU);
    MCUSR = 0U;
    wdt_disable();
}

#if CM_FEATURE_DIAGNOSTICS || CM_FEATURE_BOOT_DIAGNOSTICS
extern char __heap_start;
extern char* __brkval;
#endif
#endif

namespace
{
constexpr uint8_t KeypadRows = 4U;
constexpr uint8_t KeypadCols = 4U;
constexpr uint16_t KeypadDebounceMs = 25U;
const char KeyMap[KeypadRows * KeypadCols] PROGMEM = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};
const uint8_t RowPins[KeypadRows] = {
    CM::Pins::KeypadRow0, CM::Pins::KeypadRow1,
    CM::Pins::KeypadRow2, CM::Pins::KeypadRow3
};
const uint8_t ColPins[KeypadCols] = {
    CM::Pins::KeypadCol0, CM::Pins::KeypadCol1,
    CM::Pins::KeypadCol2, CM::Pins::KeypadCol3
};
char keypadCandidate = '\0';
char keypadStable = '\0';
uint32_t keypadChangedAtMs = 0UL;
LiquidCrystal_I2C lcd(CM::Defaults::LcdI2cAddress, 16, 2);

CM::StateMachine machine;
CM::InputController input(machine);
CM::Lcd1602View lcdView(lcd);
CM::DebouncedButton startButton(CM::Pins::StartButton,
                                true,
                                CM::Defaults::StartDebounceMs);
CM::BuzzerService buzzer(CM::Pins::Buzzer, false);
CM::SsrController ssr(CM::Pins::Ssr, true);
CM::HallTurnSource hall(CM::Pins::Hall,
                        CM::Defaults::HallThreshold,
                        CM::Defaults::HallHysteresis);
CM::SimulatedTurnSource simulator(CM::Defaults::SimulatedTurnIntervalMs);
CM::UartEventTransport espTransport(CM::Pins::EspRx,
                                    CM::Pins::EspTx,
                                    CM::Defaults::EspUartBaud);
CM::EepromPersistence persistence;
CM::HardwareSettingsStore hardwareSettingsStore;
CM::HardwareSettingsController hardwareSettingsController(hardwareSettingsStore,
                                                          hall,
                                                          machine);
CM::HallTelemetryService hallTelemetry(hall);
CM::HallCalibrationService hallCalibration(hall);
CM::HallCalibrationState previousHallCalibrationState =
    CM::HallCalibrationState::Idle;

CM::MachineState previousState = CM::MachineState::Fault;
bool synchronizationError = false;
#if CM_FEATURE_DIAGNOSTICS
uint32_t lastAliveReportMs = 0UL;
#endif
uint32_t lastHallTelemetrySendMs = 0UL;
uint8_t emergencyClearSequenceIndex = 0U;
uint32_t emergencyClearLastKeyMs = 0UL;

#if CM_FEATURE_DIAGNOSTICS || CM_FEATURE_BOOT_DIAGNOSTICS
int freeSramBytes()
{
#if defined(__AVR__)
    char stackMarker = 0;
    const uintptr_t stackAddress = reinterpret_cast<uintptr_t>(&stackMarker);
    const uintptr_t heapAddress = reinterpret_cast<uintptr_t>(
        __brkval != nullptr ? __brkval : &__heap_start);
    if (stackAddress <= heapAddress) return 0;
    return static_cast<int>(stackAddress - heapAddress);
#else
    return -1;
#endif
}
#endif

void printResetCause()
{
#if CM_FEATURE_BOOT_DIAGNOSTICS
    // Compact field format keeps the ATmega328P diagnostic image within its
    // flash limit: R=reset flags, L=last retained loop phase, M=free SRAM.
    cmBootSerial.print(F("R="));
#if defined(__AVR__)
    cmBootSerial.print(cmResetFlags, HEX);
    cmBootSerial.print(F(" L="));
    cmBootSerial.print(cmLoopPhaseMagic == 0xA5U ? cmLastLoopPhase : 0U);
#else
    cmBootSerial.print(F("-- L=0"));
#endif
    cmBootSerial.print(F(" M="));
    cmBootSerial.println(freeSramBytes());
    cmBootSerial.flush();
#endif
}

void printBootStage(const __FlashStringHelper* stage)
{
#if CM_FEATURE_BOOT_DIAGNOSTICS
    cmBootSerial.print(F("B="));
    cmBootSerial.print(stage);
    cmBootSerial.print(F(" M="));
    cmBootSerial.println(freeSramBytes());
    cmBootSerial.flush();
#else
    (void)stage;
#endif
}

void showLcdBootStage(const __FlashStringHelper* stage)
{
#if CM_FEATURE_LCD1602 && !CM_FEATURE_BOOT_DIAGNOSTICS
    lcd.setCursor(0U, 0U);
    lcd.print(F("CM BOOT RST:"));
#if defined(__AVR__)
    if (cmResetFlags < 0x10U) lcd.print('0');
    lcd.print(cmResetFlags, HEX);
#else
    lcd.print(F("--"));
#endif
    lcd.print(F("  "));
    lcd.setCursor(0U, 1U);
    lcd.print(F("                "));
    lcd.setCursor(0U, 1U);
    lcd.print(stage);
    // Keep every checkpoint visible long enough to identify the last completed
    // service during a reset loop. SSR is already initialized fail-safe OFF.
    delay(350U);
#else
    (void)stage;
#endif
}

void showPreviousLoopPhase()
{
#if CM_FEATURE_LCD1602 && defined(__AVR__) && !CM_FEATURE_BOOT_DIAGNOSTICS
    if (cmLoopPhaseMagic == 0xA5U)
    {
        lcd.setCursor(0U, 0U);
        lcd.print(F("PREV LOOP:      "));
        lcd.setCursor(0U, 1U);
        lcd.print(F("                "));
        lcd.setCursor(0U, 1U);
        lcd.print(F("PHASE "));
        lcd.print(cmLastLoopPhase);
        delay(2000U);
    }
#endif
#if defined(__AVR__)
    // Reset the retained marker only after USB diagnostics consumed the phase
    // left by the previous reset cycle.
    cmLoopPhaseMagic = 0xA5U;
    cmLastLoopPhase = 0U;
#endif
}

CM::ITurnSource& activeTurnSource()
{
#if CM_FEATURE_SIMULATION
    return simulator;
#else
    return hall;
#endif
}

bool simulationMode()
{
#if CM_FEATURE_SIMULATION
    return true;
#else
    return false;
#endif
}

bool hallCalibrationEnvironmentSafe()
{
    return machine.state() == CM::MachineState::EnterCoilCount &&
           !machine.job().isValid();
}

CM::HardwareControlResult hardwareControlResult(
    CM::HardwareSettingsApplyResult result)
{
    switch (result)
    {
        case CM::HardwareSettingsApplyResult::Applied:
            return CM::HardwareControlResult::Applied;
        case CM::HardwareSettingsApplyResult::Busy:
            return CM::HardwareControlResult::Busy;
        case CM::HardwareSettingsApplyResult::Invalid:
            return CM::HardwareControlResult::Invalid;
        case CM::HardwareSettingsApplyResult::PersistenceFailed:
        default:
            return CM::HardwareControlResult::PersistenceFailed;
    }
}

void printHardwareSettings()
{
#if CM_FEATURE_DIAGNOSTICS
    const CM::HardwareSettings& settings = hardwareSettingsController.settings();
    Serial.print(F("CM_HW_SETTINGS source="));
    Serial.print(hardwareSettingsController.loadedFromEeprom()
                     ? F("EEPROM")
                     : F("FACTORY_FALLBACK"));
    Serial.print(F(" threshold="));
    Serial.print(settings.hallThreshold);
    Serial.print(F(" hysteresis="));
    Serial.print(settings.hallHysteresis);
    Serial.print(F(" release_debounce_ms="));
    Serial.print(settings.hallReleaseDebounceMs);
    Serial.print(F(" direction="));
    Serial.println(settings.hallDirection == CM::HallSignalDirection::Falling
                       ? F("FALLING")
                       : F("RISING"));
#endif
}

void beginKeypad()
{
#if CM_FEATURE_KEYPAD_4X4
    for (uint8_t row = 0U; row < KeypadRows; ++row)
    {
        pinMode(RowPins[row], OUTPUT);
        digitalWrite(RowPins[row], HIGH);
    }
    for (uint8_t col = 0U; col < KeypadCols; ++col)
        pinMode(ColPins[col], INPUT_PULLUP);
#endif
}

char scanKeypadRaw()
{
#if CM_FEATURE_KEYPAD_4X4
    for (uint8_t row = 0U; row < KeypadRows; ++row)
    {
        digitalWrite(RowPins[row], LOW);
        delayMicroseconds(3U);
        for (uint8_t col = 0U; col < KeypadCols; ++col)
        {
            if (digitalRead(ColPins[col]) == LOW)
            {
                digitalWrite(RowPins[row], HIGH);
                const uint8_t index = static_cast<uint8_t>(row * KeypadCols + col);
                return static_cast<char>(pgm_read_byte(&KeyMap[index]));
            }
        }
        digitalWrite(RowPins[row], HIGH);
    }
#endif
    return '\0';
}

char pollKeypad()
{
#if CM_FEATURE_KEYPAD_4X4
    const char raw = scanKeypadRaw();
    const uint32_t nowMs = millis();
    if (raw != keypadCandidate)
    {
        keypadCandidate = raw;
        keypadChangedAtMs = nowMs;
        return '\0';
    }
    if (raw != keypadStable &&
        static_cast<uint32_t>(nowMs - keypadChangedAtMs) >= KeypadDebounceMs)
    {
        keypadStable = raw;
        if (keypadStable != '\0') return keypadStable;
    }
#endif
    return '\0';
}

bool processEmergencyJobClearKey(char key)
{
    static const char Sequence[] = {'D', '*', '#', 'D'};
    const uint32_t nowMs = millis();

    const CM::MachineState state = machine.state();
    if (state == CM::MachineState::Winding ||
        state == CM::MachineState::Paused ||
        state == CM::MachineState::ManualRun ||
        state == CM::MachineState::CoilComplete ||
        state == CM::MachineState::JobComplete)
    {
        emergencyClearSequenceIndex = 0U;
        return false;
    }

    if (emergencyClearSequenceIndex != 0U &&
        static_cast<uint32_t>(nowMs - emergencyClearLastKeyMs) > 4000UL)
    {
        emergencyClearSequenceIndex = 0U;
    }

    if (key != Sequence[emergencyClearSequenceIndex])
    {
        emergencyClearSequenceIndex = key == Sequence[0] ? 1U : 0U;
        emergencyClearLastKeyMs = nowMs;
        return key == Sequence[0];
    }

    ++emergencyClearSequenceIndex;
    emergencyClearLastKeyMs = nowMs;
    if (emergencyClearSequenceIndex < sizeof(Sequence)) return true;
    emergencyClearSequenceIndex = 0U;

    const CM::WindingJob& active = machine.job();
    const bool remotePresent =
        active.source == CM::JobSource::Esp32Web && active.jobId != 0UL;
    if (remotePresent)
    {
        const bool safeRemoteReady =
            machine.state() == CM::MachineState::Ready &&
            active.currentRunId == 0UL && active.completedRuns == 0U;
        if (!safeRemoteReady || !machine.cancel())
        {
            Serial.println(F("CM_JOB EMERGENCY_CLEAR result=REJECTED_ACTIVE_RUN"));
            return true;
        }
    }

    // This frame never means RUN_COMPLETED. It only proves that Arduino holds
    // no ESP32 remote job, allowing ESP32 to clear pending/accepted no-run state.
    espTransport.sendJobClear();
#if CM_FEATURE_BUZZER
    buzzer.startJobAcceptedSignal(nowMs);
#endif
    Serial.println(F("CM_JOB EMERGENCY_CLEAR result=ALL_CLEAR"));
    return true;
}

void processKeypad()
{
#if CM_FEATURE_KEYPAD_4X4
    const char key = pollKeypad();
    if (key == '\0') return;
    if (hallCalibration.active())
    {
        hallCalibration.abort();
        ssr.forceOff();
        Serial.println(F("CM_HALL_CAL abort=KEYPAD_INPUT"));
        return;
    }
    if (processEmergencyJobClearKey(key)) return;

    bool handled = false;
    if (key == '#')
    {
        CM::KeyEvent event;
        event.action = CM::InputAction::Confirm;
        handled = input.handleEvent(event);
    }
    else
    {
        handled = input.handleKey(key);
    }

#if CM_FEATURE_DIAGNOSTICS
    Serial.print(F("CM_KEY key="));
    Serial.print(key);
    Serial.print(F(" code="));
    Serial.print(static_cast<unsigned int>(static_cast<uint8_t>(key)));
    Serial.print(F(" state="));
    Serial.print(static_cast<unsigned int>(machine.state()));
    Serial.print(F(" handled="));
    Serial.println(handled ? F("1") : F("0"));
#else
    (void)handled;
#endif
#endif
}

void processExternalStart(uint32_t nowMs)
{
#if CM_FEATURE_EXTERNAL_START
    if (startButton.pollPressed(nowMs))
    {
        if (hallCalibration.state() ==
            CM::HallCalibrationState::ArmedWaitingPhysicalStart)
        {
            const bool started = hallCalibration.physicalStart(nowMs);
            Serial.println(started
                               ? F("CM_HALL_CAL physical_start=ACCEPTED")
                               : F("CM_HALL_CAL physical_start=BASELINE_NOT_READY"));
            return;
        }

        if (hallCalibration.state() == CM::HallCalibrationState::Running)
        {
            hallCalibration.abort();
            ssr.forceOff();
            Serial.println(F("CM_HALL_CAL abort=PHYSICAL_START_PRESSED_AGAIN"));
            return;
        }

        CM::KeyEvent event;
        event.action = CM::InputAction::StartOrResume;
        input.handleEvent(event);
    }
#else
    (void)nowMs;
#endif
}

void processTurnSource(uint32_t nowMs)
{
    if (machine.state() != CM::MachineState::Winding) return;
    if (activeTurnSource().pollTurn(nowMs)) machine.registerTurn();
}

void processStateTransitions(uint32_t nowMs)
{
    const CM::MachineState currentState = machine.state();
    if (currentState == previousState) return;

    if (!hardwareSettingsController.safeToChange() && hallTelemetry.enabled())
        hallTelemetry.setEnabled(false, nowMs);

    if (currentState == CM::MachineState::Winding)
        activeTurnSource().reset(nowMs);

#if CM_FEATURE_BUZZER
    if (currentState == CM::MachineState::CoilComplete)
        buzzer.startCoilCompleteSignal(nowMs);
    else if (currentState == CM::MachineState::JobComplete)
        buzzer.startProgramCompleteSignal(nowMs);
    else if (currentState == CM::MachineState::Winding ||
             currentState == CM::MachineState::ManualRun ||
             currentState == CM::MachineState::EnterCoilCount ||
             currentState == CM::MachineState::Fault)
        buzzer.stop();
#endif

    previousState = currentState;
}

void processCoreEvents()
{
    CM::WindingEvent event;
    while (machine.takeEvent(event))
    {
        persistence.saveNextIdentifiers(machine.nextSessionId(),
                                        machine.nextRunId());

        // Capture the job before any later UI action can edit/reset it. For a
        // local-keypad run this metadata is the immutable program description
        // sent to the ESP32 autonomous archive.
        const CM::WindingJob eventJob = machine.job();

        bool persisted = true;
        if (event.type == CM::WindingEventType::RunCompleted)
        {
            persisted = persistence.addPendingCompleted(event, eventJob);
        }

        const bool queued = persisted && espTransport.enqueue(event, eventJob);
        if (!queued) synchronizationError = true;

        Serial.print(F("CM_UART "));
        Serial.print(queued ? F("QUEUED") : F("QUEUE_OR_EEPROM_FULL"));
        Serial.print(F(" type="));
        Serial.print(event.type == CM::WindingEventType::RunStarted
                         ? F("RUN_STARTED")
                         : F("RUN_COMPLETED"));
        Serial.print(F(" source="));
        Serial.print(eventJob.source == CM::JobSource::LocalKeypad
                         ? F("LOCAL") : F("ESP32"));
        Serial.print(F(" session="));
        Serial.print(event.sessionId);
        Serial.print(F(" run="));
        Serial.print(event.runId);
        Serial.print(F(" pending="));
        Serial.println(espTransport.queuedCount());
    }
}

void processRemoteJobs()
{
    CM::WindingJob remoteJob;
    while (espTransport.takeRemoteJob(remoteJob))
    {
        if (hallCalibration.active())
        {
            espTransport.sendJobResult(remoteJob.jobId,
                                       false,
                                       "BUSY_CALIBRATION");
            continue;
        }

        const bool accepted = machine.loadRemoteJob(remoteJob);
        espTransport.sendJobResult(remoteJob.jobId,
                                   accepted,
                                   accepted ? "READY" : "BUSY_OR_INVALID");

#if CM_FEATURE_BUZZER
        if (accepted) buzzer.startJobAcceptedSignal(millis());
#endif

        Serial.print(F("CM_JOB RX id="));
        Serial.print(remoteJob.jobId);
        Serial.print(F(" coils="));
        Serial.print(remoteJob.coilCount);
        Serial.print(F(" repeat_target="));
        Serial.print(remoteJob.repeatTarget);
        Serial.print(F(" result="));
        Serial.println(accepted ? F("ACCEPTED") : F("REJECTED"));
    }
}

void processRemoteCancels()
{
    uint32_t jobId = 0UL;
    while (espTransport.takeRemoteCancel(jobId))
    {
        const CM::WindingJob& active = machine.job();
        const bool remotePresent =
            active.source == CM::JobSource::Esp32Web && active.jobId != 0UL;
        const bool exactReady =
            machine.state() == CM::MachineState::Ready &&
            remotePresent && active.jobId == jobId &&
            active.currentRunId == 0UL && active.completedRuns == 0U;
        const bool alreadyClear = !remotePresent;

        bool cancelled = false;
        const char* detail = "BUSY_OR_MISMATCH";
        if (exactReady)
        {
            cancelled = machine.cancel();
            detail = cancelled ? "CANCELLED" : "BUSY_OR_MISMATCH";
        }
        else if (alreadyClear)
        {
            // Idempotent cancellation: ESP32 may be retrying after Arduino
            // already cleared the job or after a reboot. Do not reject absence.
            cancelled = true;
            detail = "ALREADY_CLEAR";
        }

        espTransport.sendJobCancelResult(jobId, cancelled, detail);

        Serial.print(F("CM_JOB CANCEL id="));
        Serial.print(jobId);
        Serial.print(F(" result="));
        Serial.println(cancelled ? F("CANCELLED") : F("REJECTED"));
    }
}

void processHallCalibrationCommands(uint32_t nowMs)
{
    CM::HallCalibrationCommand command;
    while (espTransport.takeHallCalibrationCommand(command))
    {
        switch (command)
        {
            case CM::HallCalibrationCommand::Arm:
                if (hallCalibrationEnvironmentSafe())
                {
                    hallTelemetry.setEnabled(false, nowMs);
                    hallCalibration.reset();
                    hallCalibration.arm(nowMs);
                    previousHallCalibrationState = hallCalibration.state();
                }
                espTransport.sendHallCalibrationState(
                    hallCalibration.state(),
                    hallCalibration.baselineReady(),
                    hallCalibration.motorPermit());
                break;

            case CM::HallCalibrationCommand::Abort:
                hallCalibration.abort();
                ssr.forceOff();
                espTransport.sendHallCalibrationState(
                    hallCalibration.state(),
                    hallCalibration.baselineReady(),
                    hallCalibration.motorPermit());
                break;

            case CM::HallCalibrationCommand::Get:
            {
                espTransport.sendHallCalibrationState(
                    hallCalibration.state(),
                    hallCalibration.baselineReady(),
                    hallCalibration.motorPermit());
                CM::HallCalibrationResult result;
                if (hallCalibration.latestResult(result))
                    espTransport.sendHallCalibrationResult(result);
                break;
            }

            case CM::HallCalibrationCommand::None:
            default:
                break;
        }
    }
}

void processHardwareControlRequests(uint32_t nowMs)
{
    CM::HardwareControlRequest request;
    while (espTransport.takeHardwareControlRequest(request))
    {
        if (hallCalibration.active() &&
            request.type != CM::HardwareControlRequestType::GetHallSettings &&
            request.type != CM::HardwareControlRequestType::StopHallTelemetry)
        {
            espTransport.sendHardwareControlResult(
                CM::HardwareControlResult::Busy);
            continue;
        }

        switch (request.type)
        {
            case CM::HardwareControlRequestType::GetHallSettings:
                espTransport.sendHardwareSettingsState(
                    hardwareSettingsController.settings(),
                    hardwareSettingsController.loadedFromEeprom());
                break;

            case CM::HardwareControlRequestType::SetHallSettings:
            {
                const CM::HardwareSettingsApplyResult result =
                    hardwareSettingsController.apply(request.settings);
                espTransport.sendHardwareControlResult(hardwareControlResult(result));
                if (result == CM::HardwareSettingsApplyResult::Applied)
                {
                    hall.reset(nowMs);
                    hallTelemetry.resetWindow(nowMs);
                    espTransport.sendHardwareSettingsState(
                        hardwareSettingsController.settings(), true);
                    printHardwareSettings();
                }
                break;
            }

            case CM::HardwareControlRequestType::ResetHallSettings:
            {
                const CM::HardwareSettingsApplyResult result =
                    hardwareSettingsController.resetToFactoryDefaults();
                espTransport.sendHardwareControlResult(hardwareControlResult(result));
                if (result == CM::HardwareSettingsApplyResult::Applied)
                {
                    hall.reset(nowMs);
                    hallTelemetry.resetWindow(nowMs);
                    espTransport.sendHardwareSettingsState(
                        hardwareSettingsController.settings(), true);
                    printHardwareSettings();
                }
                break;
            }

            case CM::HardwareControlRequestType::StartHallTelemetry:
                if (hardwareSettingsController.safeToChange())
                {
                    hallTelemetry.setEnabled(true, nowMs);
                    lastHallTelemetrySendMs = nowMs;
                    espTransport.sendHardwareControlResult(
                        CM::HardwareControlResult::Applied);
                }
                else
                {
                    espTransport.sendHardwareControlResult(
                        CM::HardwareControlResult::Busy);
                }
                break;

            case CM::HardwareControlRequestType::StopHallTelemetry:
                hallTelemetry.setEnabled(false, nowMs);
                espTransport.sendHardwareControlResult(
                    CM::HardwareControlResult::Applied);
                break;

            case CM::HardwareControlRequestType::None:
            default:
                espTransport.sendHardwareControlResult(
                    CM::HardwareControlResult::Unsupported);
                break;
        }
    }
}

void processHallTelemetry(uint32_t nowMs)
{
    if (!hallTelemetry.enabled()) return;

    if (!hardwareSettingsController.safeToChange())
    {
        hallTelemetry.setEnabled(false, nowMs);
        return;
    }

    hallTelemetry.update(nowMs);
    if (static_cast<uint32_t>(nowMs - lastHallTelemetrySendMs) < 250UL) return;

    CM::HallTelemetrySnapshot snapshot;
    if (hallTelemetry.takeSnapshot(snapshot))
    {
        espTransport.sendHallTelemetry(snapshot);
        lastHallTelemetrySendMs = nowMs;
    }
}

void processUart(uint32_t nowMs)
{
    espTransport.update(nowMs);
    processHallCalibrationCommands(nowMs);
    processHardwareControlRequests(nowMs);
    processRemoteJobs();
    processRemoteCancels();

    CM::UartDeliveryEvent delivery;
    while (espTransport.takeDeliveryEvent(delivery))
    {
        Serial.print(F("CM_UART RX run="));
        Serial.print(delivery.runId);
        Serial.print(F(" result="));

        switch (delivery.result)
        {
            case CM::UartDeliveryResult::Acknowledged:
            {
                persistence.removePendingCompleted(delivery.runId);
                const bool terminalRemoteCleared =
                    machine.acknowledgeDeliveredRun(delivery.runId);
                synchronizationError = false;
                Serial.println(F("ACK"));
                if (terminalRemoteCleared)
                    Serial.println(F("CM_JOB AUTO_CLEAR result=FINAL_RUN_ACKED"));
                break;
            }

            case CM::UartDeliveryResult::Duplicate:
            {
                persistence.removePendingCompleted(delivery.runId);
                const bool terminalRemoteCleared =
                    machine.acknowledgeDeliveredRun(delivery.runId);
                synchronizationError = false;
                Serial.println(F("DUPLICATE"));
                if (terminalRemoteCleared)
                    Serial.println(F("CM_JOB AUTO_CLEAR result=FINAL_RUN_DUPLICATE"));
                break;
            }

            case CM::UartDeliveryResult::NegativeAcknowledgement:
                synchronizationError = true;
                Serial.println(F("NACK_RETRY"));
                break;

            case CM::UartDeliveryResult::None:
            default:
                Serial.println(F("NONE"));
                break;
        }
    }
}

void restorePersistentState()
{
    persistence.begin();
    machine.setNextIdentifiers(persistence.nextSessionId(),
                               persistence.nextRunId());

    for (uint8_t index = 0U; index < persistence.pendingCount(); ++index)
    {
        CM::WindingEvent event;
        CM::WindingJob job;
        bool hasJobMetadata = false;
        if (!persistence.pendingAt(index, event, job, hasJobMetadata))
        {
            synchronizationError = true;
            continue;
        }

        const bool queued = hasJobMetadata
            ? espTransport.enqueue(event, job)
            : espTransport.enqueue(event);
        if (!queued) synchronizationError = true;
    }

    Serial.print(F("CM_EEPROM restored_pending="));
    Serial.print(persistence.pendingCount());
    Serial.print(F(" next_session="));
    Serial.print(machine.nextSessionId());
    Serial.print(F(" next_run="));
    Serial.println(machine.nextRunId());
}

void processBuzzer(uint32_t nowMs)
{
#if CM_FEATURE_BUZZER
    buzzer.update(nowMs);
#else
    (void)nowMs;
#endif
}

void processAliveReport(uint32_t nowMs)
{
#if CM_FEATURE_DIAGNOSTICS
    if (static_cast<uint32_t>(nowMs - lastAliveReportMs) < 5000UL) return;
    lastAliveReportMs = nowMs;
    Serial.print(F("CM_ALIVE uptime_ms="));
    Serial.print(nowMs);
    Serial.print(F(" state="));
    Serial.print(static_cast<unsigned int>(machine.state()));
    Serial.print(F(" free_sram="));
    Serial.println(freeSramBytes());
#else
    (void)nowMs;
#endif
}

void processHallCalibration(uint32_t nowMs)
{
    if (hallCalibration.active())
    {
        if (hallTelemetry.enabled())
            hallTelemetry.setEnabled(false, nowMs);
        hallCalibration.update(nowMs, hallCalibrationEnvironmentSafe());
    }

    CM::HallCalibrationResult result;
    if (hallCalibration.takeResult(result))
        espTransport.sendHallCalibrationResult(result);

    if (hallCalibration.state() != previousHallCalibrationState)
    {
        previousHallCalibrationState = hallCalibration.state();
        espTransport.sendHallCalibrationState(
            hallCalibration.state(),
            hallCalibration.baselineReady(),
            hallCalibration.motorPermit());
    }

    if (hallCalibration.state() == CM::HallCalibrationState::Completed ||
        hallCalibration.state() == CM::HallCalibrationState::Aborted)
    {
        ssr.forceOff();
    }
}

void updateOutputs()
{
#if CM_FEATURE_SSR
    ssr.update(machine.state(),
               simulationMode(),
               hallCalibration.motorPermit());
#endif

#if CM_FEATURE_LCD1602
    CM::UiModel model = CM::UiModelBuilder::build(machine, input);
    model.pendingSyncCount = persistence.pendingCount();

    if (synchronizationError)
        model.syncState = CM::UiSyncState::Error;
    else if (model.pendingSyncCount > 0U || espTransport.queuedCount() > 0U)
        model.syncState = CM::UiSyncState::Pending;
    else
        model.syncState = CM::UiSyncState::Synchronized;

    lcdView.render(model);
#endif
}
} // namespace

void setup()
{
#if CM_FEATURE_SSR
    // Establish the physical safety boundary before diagnostics or services.
    ssr.begin();
#endif

#if CM_FEATURE_BOOT_DIAGNOSTICS
    cmBootSerial.begin(115200);
#else
    Serial.begin(115200);
#endif
    printResetCause();
    printBootStage(F("SERIAL"));

#if CM_FEATURE_LCD1602
    Wire.begin();
    lcdView.begin();
    showPreviousLoopPhase();
    showLcdBootStage(F("LCD"));
    printBootStage(F("LCD"));
#endif

    espTransport.begin();
    showLcdBootStage(F("UART"));
    printBootStage(F("UART"));

    restorePersistentState();
    showLcdBootStage(F("EEPROM"));
    printBootStage(F("EEPROM"));

    hardwareSettingsController.begin();
    showLcdBootStage(F("SETTINGS"));
    printHardwareSettings();
    printBootStage(F("HW_SETTINGS"));

#if CM_FEATURE_SSR
    showLcdBootStage(F("SSR SAFE OFF"));
    printBootStage(F("SSR_SAFE_OFF"));
#endif
#if CM_FEATURE_BUZZER
    buzzer.begin();
    showLcdBootStage(F("BUZZER"));
    printBootStage(F("BUZZER"));
#endif
#if CM_FEATURE_KEYPAD_4X4
    beginKeypad();
    showLcdBootStage(F("KEYPAD"));
    printBootStage(F("KEYPAD"));
#endif
#if CM_FEATURE_EXTERNAL_START
    startButton.begin();
    showLcdBootStage(F("START"));
    printBootStage(F("START_BUTTON"));
#endif
#if CM_FEATURE_SIMULATION
    simulator.setEnabled(true, millis());
    printBootStage(F("SIMULATION"));
#endif

    machine.resetToHome();
    previousState = CM::MachineState::Fault;
    processStateTransitions(millis());
    showLcdBootStage(F("STATE"));
    printBootStage(F("STATE"));

    updateOutputs();
    printBootStage(F("OUTPUTS"));
    printBootStage(F("READY"));
}

void loop()
{
    const uint32_t nowMs = millis();

#if defined(__AVR__)
    cmLastLoopPhase = 1U;
#endif
    processKeypad();
#if defined(__AVR__)
    cmLastLoopPhase = 2U;
#endif
    processExternalStart(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 3U;
#endif
    processStateTransitions(nowMs);
    processCoreEvents();

#if defined(__AVR__)
    cmLastLoopPhase = 4U;
#endif
    processTurnSource(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 5U;
#endif
    processStateTransitions(nowMs);
    processCoreEvents();

#if defined(__AVR__)
    cmLastLoopPhase = 6U;
#endif
    processUart(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 7U;
#endif
    processHallCalibration(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 8U;
#endif
    processHallTelemetry(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 9U;
#endif
    processBuzzer(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 10U;
#endif
    processStateTransitions(nowMs);

#if defined(__AVR__)
    cmLastLoopPhase = 11U;
#endif
    updateOutputs();
#if defined(__AVR__)
    cmLastLoopPhase = 12U;
#endif
    processAliveReport(nowMs);
#if defined(__AVR__)
    cmLastLoopPhase = 13U;
#endif
}
