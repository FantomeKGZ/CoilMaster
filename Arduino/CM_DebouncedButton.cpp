#include "CM_DebouncedButton.h"

namespace CM
{
DebouncedButton::DebouncedButton(uint8_t pin,
                                 bool activeLow,
                                 uint16_t debounceMs)
    : m_pin(pin),
      m_activeLow(activeLow),
      m_debounceMs(debounceMs),
      m_rawPressed(false),
      m_stablePressed(false),
      m_lastChangeMs(0UL)
{
}

void DebouncedButton::begin()
{
    pinMode(m_pin, m_activeLow ? INPUT_PULLUP : INPUT);
    m_rawPressed = readPressed();
    m_stablePressed = m_rawPressed;
    m_lastChangeMs = millis();
}

bool DebouncedButton::pollPressed(uint32_t nowMs)
{
    const bool currentRaw = readPressed();

    if (currentRaw != m_rawPressed)
    {
        m_rawPressed = currentRaw;
        m_lastChangeMs = nowMs;
    }

    if (m_stablePressed != m_rawPressed &&
        static_cast<uint32_t>(nowMs - m_lastChangeMs) >= m_debounceMs)
    {
        m_stablePressed = m_rawPressed;
        return m_stablePressed;
    }

    return false;
}

bool DebouncedButton::isPressed() const
{
    return m_stablePressed;
}

bool DebouncedButton::readPressed() const
{
    const bool high = digitalRead(m_pin) == HIGH;
    return m_activeLow ? !high : high;
}
}
