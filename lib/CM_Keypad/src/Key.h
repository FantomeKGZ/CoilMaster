/*
 * CoilMaster bounded copy of the Key class from Chris--A/Keypad 3.1.1.
 * Original library: Mark Stanley / Alexander Brevig, LGPL-2.1-or-later.
 */
#ifndef CM_BOUNDED_KEY_H
#define CM_BOUNDED_KEY_H

#include <Arduino.h>

typedef unsigned int uint;
typedef enum { IDLE, PRESSED, HOLD, RELEASED } KeyState;

const char NO_KEY = '\0';

class Key
{
public:
    char kchar;
    int kcode;
    KeyState kstate;
    boolean stateChanged;

    Key();
    explicit Key(char userKeyChar);
    void key_update(char userKeyChar, KeyState userState, boolean userStatus);
};

#endif
