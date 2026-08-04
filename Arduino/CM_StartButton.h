#ifndef CM_START_BUTTON_H
#define CM_START_BUTTON_H

#include <Arduino.h>

namespace CM
{

class StartButton
{
public:
    StartButton(uint8_t pin, uint16_t debounceMs = 40U);

    void begin();

    /** Returns true once for every stable press. */
    bool pollPressed(uint32_t nowMs);

private:
    uint8_t m_pin;
    uint16_t m_debounceMs;
    bool m_lastRawPressed;
    bool m_stablePressed;
    uint32_t m_lastChangeMs;
};

} // namespace CM

#endif // CM_START_BUTTON_H
