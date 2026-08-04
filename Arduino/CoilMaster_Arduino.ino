#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#include "Config/CM_Features.h"
#include "Config/CM_Pins.h"

#include "../Core/CM_InputController.h"
#include "../Core/CM_StateMachine.h"
#include "../Core/CM_TurnSource.h"
#include "../Core/CM_UiModel.h"

#include "CM_BuzzerService.h"
#include "CM_DebouncedButton.h"
#include "CM_HallTurnSource.h"
#include "CM_Lcd1602View.h"
#include "CM_SsrController.h"

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
LiquidCrystal_I2C lcd(CM::Pins::LcdI2cAddress, 16, 2);

CM::StateMachine machine;
CM::InputController input(machine);
CM::Lcd1602View lcdView(lcd);
CM::DebouncedButton startButton(CM::Pins::StartButton, true, 40U);
CM::BuzzerService buzzer(CM::Pins::Buzzer, true);
CM::SsrController ssr(CM::Pins::Ssr, true);
CM::HallTurnSource hall(CM::Pins::Hall, 590U, 50U);
CM::SimulatedTurnSource simulator(250U);

CM::MachineState previousState = CM::MachineState::EnterCoilCount;

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

    if (currentState == CM::MachineState::CoilComplete)
    {
#if CM_FEATURE_BUZZER
        buzzer.startCompletionSignal(nowMs);
#else
        machine.acknowledgeCoilComplete();
#endif
    }

    if (currentState == CM::MachineState::EnterCoilCount ||
        currentState == CM::MachineState::Fault)
    {
        buzzer.stop();
    }

    previousState = currentState;
}

void processBuzzer(uint32_t nowMs)
{
#if CM_FEATURE_BUZZER
    buzzer.update(nowMs);
    if (buzzer.takeFinishedEvent() &&
        machine.state() == CM::MachineState::CoilComplete)
    {
        machine.acknowledgeCoilComplete();
    }
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
}

void setup()
{
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
    previousState = machine.state();
    updateOutputs();
}

void loop()
{
    const uint32_t nowMs = millis();

    processKeypad();
    processExternalStart(nowMs);
    processTurnSource(nowMs);
    processStateTransitions(nowMs);
    processBuzzer(nowMs);
    processStateTransitions(nowMs);
    updateOutputs();
}
