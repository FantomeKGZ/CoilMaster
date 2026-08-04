#include "CM_KeyMapper.h"

namespace CM
{

KeyEvent mapKey(char key)
{
    KeyEvent event;

    if (key >= '0' && key <= '9')
    {
        event.action = InputAction::Digit;
        event.digit = static_cast<uint8_t>(key - '0');
        return event;
    }

    switch (key)
    {
        case 'A':
            event.action = InputAction::StartOrResume;
            break;
        case 'B':
            event.action = InputAction::ReturnHome;
            break;
        case 'C':
            event.action = InputAction::ToggleManual;
            break;
        case 'D':
            event.action = InputAction::Pause;
            break;
        case '*':
            event.action = InputAction::Backspace;
            break;
        case '#':
            event.action = InputAction::Confirm;
            break;
        default:
            event.action = InputAction::None;
            break;
    }

    return event;
}

} // namespace CM
