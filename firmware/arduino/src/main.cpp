#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

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

CM::MachineState previousState = CM::MachineState::Fault;

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

void processKeypad()
{
#if CM_FEATURE_KEYPAD_4X4
    const char key = keypad.getKey();
    if (key != NO_KEY)
    {
        input.handleKey(key);
    }
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
    if (machine.state() != CM::MachineState::Winding)
    {
        return;
    }

    if (activeTurnSource().pollTurn(nowMs))
    {
        machine.registerTurn();
    }
}

void processStateTransitions(uint32_t nowMs)
{
    const CM::MachineState currentState = machine.state();

    if (currentState == previousState)
    {
        return;
    }

    if (currentState == CM::MachineState::Winding)
    {
        activeTurnSource().reset(nowMs);
    }

#if CM_FEATURE_BUZZER
    if (currentState == CM::MachineState::CoilComplete)
    {
        buzzer.startCoilCompleteSignal(nowMs);
    }
    else if (currentState == CM::MachineState::JobComplete)
    {
        buzzer.startProgramCompleteSignal(nowMs);
    }
#endif

    if (currentState == CM::MachineState::EnterCoilCount ||
        currentState == CM::MachineState::Fault)
    {
        buzzer.stop();
    }

    previousState = currentState;
}

void processCoreEvents()
{
    CM::WindingEvent event;
    while (machine.takeEvent(event))
    {
        // Preserve monotonic IDs before any possible loss of power.
        persistence.saveNextIdentifiers(machine.nextSessionId(),
                                        machine.nextRunId());

        bool persisted = true;
        if (event.type == CM::WindingEventType::RunCompleted)
        {
            persisted = persistence.addPendingCompleted(event);
        }

        const bool queued = persisted && espTransport.enqueue(event);

        Serial.print(F("CM_UART "));
        Serial.print(queued ? F("QUEUED") : F("QUEUE_OR_EEPROM_FULL"));
        Serial.print(F(" type="));
        Serial.print(event.type == CM::WindingEventType::RunStarted
                         ? F("RUN_STARTED")
                         : F("RUN_COMPLETED"));
        Serial.print(F(" session="));
        Serial.print(event.sessionId);
        Serial.print(F(" run="));
        Serial.print(event.runId);
        Serial.print(F(" pending="));
        Serial.println(espTransport.queuedCount());
    }
}

void processUart(uint32_t nowMs)
{
    espTransport.update(nowMs);

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
                Serial.println(F("ACK"));
                break;

            case CM::UartDeliveryResult::Duplicate:
                persistence.removePendingCompleted(delivery.runId);
                Serial.println(F("DUPLICATE"));
                break;

            case CM::UartDeliveryResult::NegativeAcknowledgement:
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

    CM::WindingEvent event;
    for (uint8_t index = 0U; index < persistence.pendingCount(); ++index)
    {
        if (persistence.pendingAt(index, event))
        {
            espTransport.enqueue(event);
        }
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

void updateOutputs()
{
#if CM_FEATURE_SSR
    ssr.update(machine.state(), simulationMode());
#endif

#if CM_FEATURE_LCD1602
    lcdView.render(CM::UiModelBuilder::build(machine, input));
#endif
}
} // namespace

void setup()
{
    Serial.begin(115200);
    espTransport.begin();
    restorePersistentState();

#if CM_FEATURE_SSR
    ssr.begin();
#endif

#if CM_FEATURE_BUZZER
    buzzer.begin();
#endif

#if CM_FEATURE_EXTERNAL_START
    startButton.begin();
#endif

#if CM_FEATURE_LCD1602
    lcdView.begin();
#endif

#if CM_FEATURE_SIMULATION
    simulator.setEnabled(true, millis());
#endif

    machine.resetToHome();
    previousState = CM::MachineState::Fault;
    processStateTransitions(millis());
    updateOutputs();
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
}
