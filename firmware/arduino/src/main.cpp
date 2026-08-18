#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

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
#include "../../../Arduino/CM_HallTurnSource.h"
#include "../../../Arduino/CM_Lcd1602View.h"
#include "../../../Arduino/CM_SsrController.h"
#include "../../../Arduino/CM_UartEventTransport.h"

#if defined(__AVR__)
uint8_t cmResetFlags __attribute__((section(".noinit")));

void cmCaptureResetFlags() __attribute__((naked, section(".init3")));
void cmCaptureResetFlags()
{
    cmResetFlags = MCUSR;
    MCUSR = 0U;
    wdt_disable();
}

extern char __heap_start;
extern char* __brkval;
#endif

namespace
{
char KeyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte RowPins[4] = {
    CM::Pins::KeypadRow0,
    CM::Pins::KeypadRow1,
    CM::Pins::KeypadRow2,
    CM::Pins::KeypadRow3
};

byte ColPins[4] = {
    CM::Pins::KeypadCol0,
    CM::Pins::KeypadCol1,
    CM::Pins::KeypadCol2,
    CM::Pins::KeypadCol3
};

Keypad keypad = Keypad(makeKeymap(KeyMap), RowPins, ColPins, 4, 4);
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

CM::MachineState previousState = CM::MachineState::Fault;
bool synchronizationError = false;
uint32_t lastAliveReportMs = 0UL;
uint8_t emergencyClearSequenceIndex = 0U;
uint32_t emergencyClearLastKeyMs = 0UL;

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

void printResetCause()
{
#if CM_FEATURE_DIAGNOSTICS
    Serial.print(F("CM_BOOT reset_flags=0x"));
#if defined(__AVR__)
    Serial.print(cmResetFlags, HEX);
    Serial.print(F(" cause="));
    bool reported = false;
#ifdef PORF
    if ((cmResetFlags & _BV(PORF)) != 0U)
    {
        Serial.print(F("POWER_ON"));
        reported = true;
    }
#endif
#ifdef EXTRF
    if ((cmResetFlags & _BV(EXTRF)) != 0U)
    {
        if (reported) Serial.print('|');
        Serial.print(F("EXTERNAL_RESET"));
        reported = true;
    }
#endif
#ifdef BORF
    if ((cmResetFlags & _BV(BORF)) != 0U)
    {
        if (reported) Serial.print('|');
        Serial.print(F("BROWN_OUT"));
        reported = true;
    }
#endif
#ifdef WDRF
    if ((cmResetFlags & _BV(WDRF)) != 0U)
    {
        if (reported) Serial.print('|');
        Serial.print(F("WATCHDOG"));
        reported = true;
    }
#endif
    if (!reported) Serial.print(F("NONE_OR_BOOTLOADER_CLEARED"));
#else
    Serial.print(F("NA cause=UNSUPPORTED"));
#endif
    Serial.println();
    Serial.flush();
#endif
}

void printBootStage(const __FlashStringHelper* stage)
{
#if CM_FEATURE_DIAGNOSTICS
    Serial.print(F("CM_BOOT stage="));
    Serial.print(stage);
    Serial.print(F(" free_sram="));
    Serial.println(freeSramBytes());
    Serial.flush();
#else
    (void)stage;
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
    const char key = keypad.getKey();
    if (key == NO_KEY) return;
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
#endif
#endif
}

void processExternalStart(uint32_t nowMs)
{
#if CM_FEATURE_EXTERNAL_START
    if (startButton.pollPressed(nowMs))
    {
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

void processUart(uint32_t nowMs)
{
    espTransport.update(nowMs);
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
                persistence.removePendingCompleted(delivery.runId);
                synchronizationError = false;
                Serial.println(F("ACK"));
                break;

            case CM::UartDeliveryResult::Duplicate:
                persistence.removePendingCompleted(delivery.runId);
                synchronizationError = false;
                Serial.println(F("DUPLICATE"));
                break;

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

void updateOutputs()
{
#if CM_FEATURE_SSR
    ssr.update(machine.state(), simulationMode());
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
    Serial.begin(115200);
    printResetCause();
    printBootStage(F("SERIAL"));

    espTransport.begin();
    printBootStage(F("UART"));

    restorePersistentState();
    printBootStage(F("EEPROM"));

#if CM_FEATURE_SSR
    ssr.begin();
    printBootStage(F("SSR"));
#endif
#if CM_FEATURE_BUZZER
    buzzer.begin();
    printBootStage(F("BUZZER"));
#endif
#if CM_FEATURE_EXTERNAL_START
    startButton.begin();
    printBootStage(F("START_BUTTON"));
#endif
#if CM_FEATURE_LCD1602
    lcdView.begin();
    printBootStage(F("LCD"));
#endif
#if CM_FEATURE_SIMULATION
    simulator.setEnabled(true, millis());
    printBootStage(F("SIMULATION"));
#endif

    machine.resetToHome();
    previousState = CM::MachineState::Fault;
    processStateTransitions(millis());
    printBootStage(F("STATE"));

    updateOutputs();
    printBootStage(F("OUTPUTS"));
    printBootStage(F("READY"));
}

void loop()
{
    const uint32_t nowMs = millis();

    processKeypad();
    processExternalStart(nowMs);
    processStateTransitions(nowMs);
    processCoreEvents();

    processTurnSource(nowMs);
    processStateTransitions(nowMs);
    processCoreEvents();

    processUart(nowMs);
    processBuzzer(nowMs);
    processStateTransitions(nowMs);

    updateOutputs();
    processAliveReport(nowMs);
}
