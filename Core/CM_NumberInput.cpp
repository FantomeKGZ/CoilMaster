#include "CM_NumberInput.h"

#include <limits.h>

namespace CM
{

NumberInput::NumberInput()
    : m_value(0U),
      m_digitCount(0U)
{
}

void NumberInput::clear()
{
    m_value = 0U;
    m_digitCount = 0U;
}

bool NumberInput::appendDigit(uint8_t digit)
{
    if (digit > 9U || m_digitCount >= MaxDigits)
    {
        return false;
    }

    const uint16_t maxBeforeMultiply = static_cast<uint16_t>((UINT16_MAX - digit) / 10U);
    if (m_value > maxBeforeMultiply)
    {
        return false;
    }

    m_value = static_cast<uint16_t>((m_value * 10U) + digit);
    ++m_digitCount;
    return true;
}

bool NumberInput::backspace()
{
    if (m_digitCount == 0U)
    {
        return false;
    }

    m_value = static_cast<uint16_t>(m_value / 10U);
    --m_digitCount;
    return true;
}

bool NumberInput::hasValue() const
{
    return m_digitCount > 0U;
}

uint16_t NumberInput::value() const
{
    return m_value;
}

uint8_t NumberInput::digitCount() const
{
    return m_digitCount;
}

bool NumberInput::isInRange(uint16_t minimum, uint16_t maximum) const
{
    return hasValue() && minimum <= maximum && m_value >= minimum && m_value <= maximum;
}

} // namespace CM
