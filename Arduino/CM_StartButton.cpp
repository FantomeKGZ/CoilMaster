#include "CM_StartButton.h"

namespace CM
{

StartButton::StartButton(uint8_t pin, uint16_t debounceMs)
    : m_pin(pin),
      m_debounceMs(debounceMs),
      m_lastRawPressed(false),
      m_stablePressed(false),
      m_lastChangeMs(0UL)
{
}

void StartButton::begin()
{
    pinMode(m_pin, INPUT_PULLUP);
    const bool pressed = digitalRead(m_pin) == LOW;
    m_lastRawPressed = pressed;
    m_stablePressed = pressed;
    m_lastChangeMs = millis();
}

bool StartButton::pollPressed(uint32_t nowMs)
{
    const bool rawPressed = digitalRead(m_pin) == LOW;

    if (rawPressed != m_lastRawPressed)
    {
        m_lastRawPressed = rawPressed;
        m_lastChangeMs = nowMs;
    }

    if ((nowMs - m_lastChangeMs) < m_debounceMs ||
        rawPressed == m_stablePressed)
    {
        return false;
    }

    m_stablePressed = rawPressed;
    return m_stablePressed;
}

} // namespace CM
