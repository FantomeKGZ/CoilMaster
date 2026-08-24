/*
 * CoilMaster bounded copy of Chris--A/Keypad 3.1.1.
 * Original library: Mark Stanley / Alexander Brevig, LGPL-2.1-or-later.
 * Scan/debounce/state semantics are preserved; capacity is bounded to the
 * production 4x4 keypad and the single-key getKey() API used by CoilMaster.
 */
#ifndef CM_BOUNDED_KEYPAD_H
#define CM_BOUNDED_KEYPAD_H

#include "Key.h"

#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif

#define OPEN LOW
#define CLOSED HIGH
#define LIST_MAX 1
#define MAPSIZE 4
#define makeKeymap(x) ((char*)x)

typedef char KeypadEvent;
typedef unsigned long ulong;

typedef struct {
    byte rows;
    byte columns;
} KeypadSize;

class Keypad : public Key
{
public:
    Keypad(char* userKeymap, byte* row, byte* col, byte numRows, byte numCols);

    virtual void pin_mode(byte pinNum, byte mode) { pinMode(pinNum, mode); }
    virtual void pin_write(byte pinNum, boolean level) { digitalWrite(pinNum, level); }
    virtual int pin_read(byte pinNum) { return digitalRead(pinNum); }

    uint bitMap[MAPSIZE];
    Key key[LIST_MAX];
    unsigned long holdTimer;

    char getKey();
    bool getKeys();
    KeyState getState();
    void begin(char* userKeymap);
    bool isPressed(char keyChar);
    void setDebounceTime(uint debounce);
    void setHoldTime(uint hold);
    void addEventListener(void (*listener)(char));
    int findInList(char keyChar);
    int findInList(int keyCode);
    char waitForKey();
    bool keyStateChanged();
    byte numKeys();

private:
    unsigned long startTime;
    char* keymap;
    byte* rowPins;
    byte* columnPins;
    KeypadSize sizeKpd;
    uint debounceTime;
    uint holdTime;
    bool single_key;

    void scanKeys();
    bool updateList();
    void nextKeyState(byte n, boolean button);
    void transitionTo(byte n, KeyState nextState);
    void (*keypadEventListener)(char);
};

#endif
