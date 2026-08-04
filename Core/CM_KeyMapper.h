#ifndef CM_KEY_MAPPER_H
#define CM_KEY_MAPPER_H

#include "CM_Types.h"

#include <stdint.h>

namespace CM
{

struct KeyEvent
{
    InputAction action;
    uint8_t digit;

    KeyEvent()
        : action(InputAction::None),
          digit(0U)
    {
    }
};

/** Convert a raw 4x4 keypad character into a hardware-independent action. */
KeyEvent mapKey(char key);

} // namespace CM

#endif // CM_KEY_MAPPER_H
