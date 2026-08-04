#ifndef CM_DEBOUNCED_BUTTON_H
#define CM_DEBOUNCED_BUTTON_H

#include <Arduino.h>

namespace CM
{
class DebouncedButton
{
public:
    DebouncedButton(uint8_t pin,
                    bool activeLow = true,
                    uint16_t debounceMs = 40U);

    void begin();

    /** Return true once for every stable press. */
    bool pollPressed(uint32_t nowMs);

    bool isPressed() const;

private:
    bool readPressed() const;

    uint8_t m_pin;
    bool m_activeLow;
    uint16_t m_debounceMs;
    bool m_rawPressed;
    bool m_stablePressed;
    uint32_t m_lastChangeMs;
};
}

#endif // CM_DEBOUNCED_BUTTON_H
