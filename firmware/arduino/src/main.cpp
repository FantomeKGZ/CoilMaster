#include <Arduino.h>
#include <Keypad.h>
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
uint8_t cmResetFlags __attribute__((section(".noinit")));
volatile uint8_t cmLastLoopPhase __attribute__((section(".noinit")));
volatile uint8_t cmLoopPhaseMagic __attribute__((section(".noinit")));

void cmCaptureResetFlags() __attribute__((naked, section(".init3")));
void cmCaptureResetFlags()
{
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
CM::BuzzerService buzzer(CM::Pins::Buzzer, true);
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
CM::HallCalibrationService hallCalibration(hall);
CM::HallCalibrationState previousHallCalibrationState =
    CM::HallCalibrationState::Idle;
CM::HardwareSettings pendingHallCalibrationSettings;
uint32_t pendingHallCalibrationMeasurementId = 0UL;
bool hasPendingHallCalibrationProposal = false;

CM::MachineState previousState = CM::MachineState::Fault;
bool synchronizationError = false;
bool hallTelemetryEnabled = false;
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

void clearPendingHallCalibrationProposal()
{
    pendingHallCalibrationSettings = CM::HardwareSettings();
    pendingHallCalibrationMeasurementId = 0UL;
    hasPendingHallCalibrationProposal = false;
}

void printResetCause()
{
#if CM_FEATURE_BOOT_DIAGNOSTICS
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

bool hardwareControlResult(int) { return false; }

} // namespace

// NOTE: remainder of file unchanged in repository baseline.
