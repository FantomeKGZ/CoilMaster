#ifndef CM_INPUT_CONTROLLER_H
#define CM_INPUT_CONTROLLER_H

#include <stdint.h>

#include "CM_KeyMapper.h"
#include "CM_NumberInput.h"
#include "CM_StateMachine.h"

namespace CM
{

/**
 * @brief Connects keypad actions, numeric input and the machine state.
 *
 * This class contains no Arduino, Keypad or LCD dependencies. Hardware code
 * only passes raw key characters to handleKey().
 */
class InputController
{
public:
    explicit InputController(StateMachine& stateMachine);

    /** Process one raw keypad character. */
    bool handleKey(char key);

    /** Process an already decoded action, useful for external START button. */
    bool handleEvent(const KeyEvent& event);

    const NumberInput& numberInput() const;
    uint8_t editingCoilIndex() const;

    /** Clear only the current numeric entry. */
    void clearEntry();

    /** Reset controller and state machine to the home screen. */
    void resetToHome();

private:
    bool handleDigit(uint8_t digit);
    bool confirmNumber();

    StateMachine& m_stateMachine;
    NumberInput m_numberInput;
    uint8_t m_editingCoilIndex;
};

} // namespace CM

#endif // CM_INPUT_CONTROLLER_H
