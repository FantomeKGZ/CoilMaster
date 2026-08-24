/*
 * CoilMaster bounded copy of Chris--A/Keypad 3.1.1.
 * Original library: Mark Stanley / Alexander Brevig, LGPL-2.1-or-later.
 */
#include <Keypad.h>

Key::Key()
    : kchar(NO_KEY), kcode(-1), kstate(IDLE), stateChanged(false)
{
}

Key::Key(char userKeyChar)
    : kchar(userKeyChar), kcode(-1), kstate(IDLE), stateChanged(false)
{
}

void Key::key_update(char userKeyChar, KeyState userState, boolean userStatus)
{
    kchar = userKeyChar;
    kstate = userState;
    stateChanged = userStatus;
}

Keypad::Keypad(char* userKeymap, byte* row, byte* col, byte numRows, byte numCols)
    : holdTimer(0UL),
      startTime(0UL),
      keymap(nullptr),
      rowPins(row),
      columnPins(col),
      sizeKpd{numRows, numCols},
      debounceTime(10U),
      holdTime(500U),
      single_key(false),
      keypadEventListener(nullptr)
{
    begin(userKeymap);
}

void Keypad::begin(char* userKeymap)
{
    keymap = userKeymap;
}

char Keypad::getKey()
{
    single_key = true;
    if (getKeys() && key[0].stateChanged && key[0].kstate == PRESSED)
        return key[0].kchar;

    single_key = false;
    return NO_KEY;
}

bool Keypad::getKeys()
{
    bool keyActivity = false;
    if ((millis() - startTime) > debounceTime)
    {
        scanKeys();
        keyActivity = updateList();
        startTime = millis();
    }
    return keyActivity;
}

void Keypad::scanKeys()
{
    for (byte r = 0U; r < sizeKpd.rows; ++r)
        pin_mode(rowPins[r], INPUT_PULLUP);

    for (byte c = 0U; c < sizeKpd.columns; ++c)
    {
        pin_mode(columnPins[c], OUTPUT);
        pin_write(columnPins[c], LOW);
        for (byte r = 0U; r < sizeKpd.rows; ++r)
            bitWrite(bitMap[r], c, !pin_read(rowPins[r]));
        pin_write(columnPins[c], HIGH);
        pin_mode(columnPins[c], INPUT);
    }
}

bool Keypad::updateList()
{
    bool anyActivity = false;

    for (byte i = 0U; i < LIST_MAX; ++i)
    {
        if (key[i].kstate == IDLE)
        {
            key[i].kchar = NO_KEY;
            key[i].kcode = -1;
            key[i].stateChanged = false;
        }
    }

    for (byte r = 0U; r < sizeKpd.rows; ++r)
    {
        for (byte c = 0U; c < sizeKpd.columns; ++c)
        {
            const boolean button = bitRead(bitMap[r], c);
            const char keyChar = keymap[r * sizeKpd.columns + c];
            const int keyCode = r * sizeKpd.columns + c;
            const int idx = findInList(keyCode);
            if (idx > -1) nextKeyState(static_cast<byte>(idx), button);
            if (idx == -1 && button)
            {
                for (byte i = 0U; i < LIST_MAX; ++i)
                {
                    if (key[i].kchar == NO_KEY)
                    {
                        key[i].kchar = keyChar;
                        key[i].kcode = keyCode;
                        key[i].kstate = IDLE;
                        nextKeyState(i, button);
                        break;
                    }
                }
            }
        }
    }

    for (byte i = 0U; i < LIST_MAX; ++i)
        if (key[i].stateChanged) anyActivity = true;
    return anyActivity;
}

void Keypad::nextKeyState(byte idx, boolean button)
{
    key[idx].stateChanged = false;
    switch (key[idx].kstate)
    {
        case IDLE:
            if (button == CLOSED)
            {
                transitionTo(idx, PRESSED);
                holdTimer = millis();
            }
            break;
        case PRESSED:
            if ((millis() - holdTimer) > holdTime)
                transitionTo(idx, HOLD);
            else if (button == OPEN)
                transitionTo(idx, RELEASED);
            break;
        case HOLD:
            if (button == OPEN) transitionTo(idx, RELEASED);
            break;
        case RELEASED:
            transitionTo(idx, IDLE);
            break;
    }
}

bool Keypad::isPressed(char keyChar)
{
    for (byte i = 0U; i < LIST_MAX; ++i)
        if (key[i].kchar == keyChar && key[i].kstate == PRESSED && key[i].stateChanged)
            return true;
    return false;
}

int Keypad::findInList(char keyChar)
{
    for (byte i = 0U; i < LIST_MAX; ++i)
        if (key[i].kchar == keyChar) return i;
    return -1;
}

int Keypad::findInList(int keyCode)
{
    for (byte i = 0U; i < LIST_MAX; ++i)
        if (key[i].kcode == keyCode) return i;
    return -1;
}

char Keypad::waitForKey()
{
    char waitKey = NO_KEY;
    while ((waitKey = getKey()) == NO_KEY) {}
    return waitKey;
}

KeyState Keypad::getState()
{
    return key[0].kstate;
}

bool Keypad::keyStateChanged()
{
    return key[0].stateChanged;
}

byte Keypad::numKeys()
{
    return static_cast<byte>(sizeof(key) / sizeof(Key));
}

void Keypad::setDebounceTime(uint debounce)
{
    debounceTime = debounce < 1U ? 1U : debounce;
}

void Keypad::setHoldTime(uint hold)
{
    holdTime = hold;
}

void Keypad::addEventListener(void (*listener)(char))
{
    keypadEventListener = listener;
}

void Keypad::transitionTo(byte idx, KeyState nextState)
{
    key[idx].kstate = nextState;
    key[idx].stateChanged = true;
    if (single_key)
    {
        if (keypadEventListener != nullptr && idx == 0U)
            keypadEventListener(key[0].kchar);
    }
    else if (keypadEventListener != nullptr)
    {
        keypadEventListener(key[idx].kchar);
    }
}
