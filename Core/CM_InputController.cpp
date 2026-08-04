#include "CM_InputController.h"

namespace CM
{

InputController::InputController(StateMachine& stateMachine)
    : m_stateMachine(stateMachine),
      m_numberInput(),
      m_editingCoilIndex(0U)
{
}

bool InputController::handleKey(char key)
{
    return handleEvent(mapKey(key));
}

bool InputController::handleEvent(const KeyEvent& event)
{
    switch (event.action)
    {
        case InputAction::Digit:
            return handleDigit(event.digit);

        case InputAction::Backspace:
            return m_numberInput.backspace();

        case InputAction::Confirm:
            return confirmNumber();

        case InputAction::StartOrResume:
            return m_stateMachine.startOrResume();

        case InputAction::Pause:
            return m_stateMachine.pause();

        case InputAction::ToggleManual:
            return m_stateMachine.toggleManual();

        case InputAction::ReturnHome:
            resetToHome();
            return true;

        case InputAction::None:
        default:
            return false;
    }
}

const NumberInput& InputController::numberInput() const
{
    return m_numberInput;
}

uint8_t InputController::editingCoilIndex() const
{
    return m_editingCoilIndex;
}

void InputController::clearEntry()
{
    m_numberInput.clear();
}

void InputController::resetToHome()
{
    m_stateMachine.resetToHome();
    m_numberInput.clear();
    m_editingCoilIndex = 0U;
}

bool InputController::handleDigit(uint8_t digit)
{
    const MachineState state = m_stateMachine.state();

    if (state != MachineState::EnterCoilCount &&
        state != MachineState::EnterTurns)
    {
        return false;
    }

    return m_numberInput.appendDigit(digit);
}

bool InputController::confirmNumber()
{
    const MachineState state = m_stateMachine.state();

    if (state == MachineState::EnterCoilCount)
    {
        if (!m_numberInput.isInRange(1U, MaxCoilsPerJob))
        {
            return false;
        }

        const uint8_t coilCount = static_cast<uint8_t>(m_numberInput.value());
        if (!m_stateMachine.setCoilCount(coilCount))
        {
            return false;
        }

        m_numberInput.clear();
        m_editingCoilIndex = 0U;
        return true;
    }

    if (state == MachineState::EnterTurns)
    {
        if (!m_numberInput.isInRange(1U, MaxTurnsPerCoil))
        {
            return false;
        }

        if (!m_stateMachine.setCoilTurns(m_editingCoilIndex,
                                         m_numberInput.value()))
        {
            return false;
        }

        m_numberInput.clear();

        if (m_stateMachine.state() == MachineState::EnterTurns)
        {
            ++m_editingCoilIndex;
        }

        return true;
    }

    return false;
}

} // namespace CM
