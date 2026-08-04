#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#include "../Config/CM_Pins.h"
#include "../CM_Buzzer.h"
#include "../CM_HallTurnSource.h"
#include "../CM_Lcd1602View.h"
#include "../CM_SsrController.h"
#include "../CM_StartButton.h"
#include "../../Core/CM_InputController.h"
#include "../../Core/CM_StateMachine.h"
#include "../../Core/CM_TurnSource.h"
#include "../../Core/CM_UiModel.h"

namespace
{
constexpr byte Rows = 4U;
constexpr byte Cols = 4U;

char keyMap[Rows][Cols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[Rows] = {
    CM::Pins::KeypadRow0,
    CM::Pins::KeypadRow1,
    CM::Pins::KeypadRow2,
    CM::Pins::KeypadRow3
};

byte colPins[Cols] = {
    CM::Pins::KeypadCol0,
    CM::Pins::KeypadCol1,
    CM::Pins::KeypadCol2,
    CM::Pins::KeypadCol3
};

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, Rows, Cols);
LiquidCrystal_I2C lcd(CM::Defaults::LcdI2cAddress, 16, 2);

CM::StateMachine stateMachine;
CM::InputController inputController(stateMachine);
CM::Lcd1602View lcdView(lcd);
CM::HallTurnSource hallSource(CM::Pins::Hall,
                              CM::Defaults::HallThreshold,
                              CM::Defaults::HallHysteresis);
CM::SimulatedTurnSource simulatedSource(CM::Defaults::SimulatedTurnIntervalMs);
CM::SsrController ssr(CM::Pins::Ssr, true);
CM::StartButton startButton(CM::Pins::StartButton,
                            CM::Defaults::StartDebounceMs);
CM::Buzzer buzzer(CM::Pins::Buzzer, true);

// Keep false for real hardware. In simulation the state machine counts turns,
// but SsrController keeps the physical SSR output disabled.
bool simulationMode = false;
CM::MachineState previousState = CM::MachineState::EnterCoilCount;

CM::ITurnSource& activeTurnSource()
{
    return simulationMode
        ? static_cast<CM::ITurnSource&>(simulatedSource)
        : static_cast<CM::ITurnSource&>(hallSource);
}

void processInputs(uint32_t nowMs)
{
    const char key = keypad.getKey();
    if (key != NO_KEY)
    {
        inputController.handleKey(key);
    }

    if (startButton.pollPressed(nowMs))
    {
        CM::KeyEvent startEvent;
        startEvent.action = CM::InputAction::StartOrResume;
        inputController.handleEvent(startEvent);
    }
}

void processStateTransition(uint32_t nowMs)
{
    const CM::MachineState currentState = stateMachine.state();
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
        buzzer.startCompletionPattern(nowMs);
    }

    if (currentState == CM::MachineState::EnterCoilCount ||
        currentState == CM::MachineState::Fault)
    {
        buzzer.stop();
    }

    previousState = currentState;
}

void processTurns(uint32_t nowMs)
{
    if (stateMachine.state() != CM::MachineState::Winding)
    {
        return;
    }

    if (activeTurnSource().pollTurn(nowMs))
    {
        stateMachine.registerTurn();
    }
}

void processCoilCompletion(uint32_t nowMs)
{
    buzzer.update(nowMs);

    if (stateMachine.state() == CM::MachineState::CoilComplete &&
        buzzer.consumeFinishedEvent())
    {
        stateMachine.acknowledgeCoilComplete();
    }
}

void updateOutputs()
{
    ssr.update(stateMachine.state(), simulationMode);
    lcdView.render(CM::UiModelBuilder::build(stateMachine, inputController));
}
}

void setup()
{
    ssr.begin();
    buzzer.begin();
    startButton.begin();
    lcdView.begin();

    const uint32_t nowMs = millis();
    hallSource.reset(nowMs);
    simulatedSource.setEnabled(simulationMode, nowMs);
    stateMachine.resetToHome();
    previousState = stateMachine.state();
    updateOutputs();
}

void loop()
{
    const uint32_t nowMs = millis();

    processInputs(nowMs);
    processTurns(nowMs);
    processStateTransition(nowMs);
    processCoilCompletion(nowMs);
    processStateTransition(nowMs);
    updateOutputs();
}
