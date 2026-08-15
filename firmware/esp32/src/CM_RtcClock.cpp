#include "CM_RtcClock.h"

namespace CM
{
namespace
{
bool decodeBcd(uint8_t source, uint8_t& value)
{
    const uint8_t low = source & 0x0FU;
    const uint8_t high = static_cast<uint8_t>((source >> 4U) & 0x0FU);
    if (low > 9U || high > 9U) return false;
    value = static_cast<uint8_t>(high * 10U + low);
    return true;
}

bool leapYear(uint16_t year)
{
    return (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
}

bool validDateTime(const RtcDateTime& value)
{
    if (value.year < 2000U || value.year > 2099U ||
        value.month < 1U || value.month > 12U || value.day < 1U ||
        value.hour > 23U || value.minute > 59U || value.second > 59U)
        return false;
    static const uint8_t daysPerMonth[] =
        {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint8_t maximumDay = daysPerMonth[value.month - 1U];
    if (value.month == 2U && leapYear(value.year)) maximumDay = 29U;
    return value.day <= maximumDay;
}
}

bool RtcClock::begin(int8_t sdaPin, int8_t sclPin)
{
    m_wire = &Wire;
    m_wire->begin(sdaPin, sclPin);
    RtcDateTime ignored;
    read(ignored);
    return m_detected;
}

bool RtcClock::read(RtcDateTime& value)
{
    value = RtcDateTime();
    if (m_wire == nullptr)
    {
        m_detected = false;
        m_timeValid = false;
        return false;
    }

    uint8_t status = 0U;
    if (!readRegisters(0x0FU, &status, 1U))
    {
        m_detected = false;
        m_timeValid = false;
        return false;
    }
    m_detected = true;
    if ((status & 0x80U) != 0U)
    {
        m_timeValid = false;
        return false;
    }

    uint8_t registers[7] = {};
    if (!readRegisters(0x00U, registers,
                       static_cast<uint8_t>(sizeof(registers))))
    {
        m_timeValid = false;
        return false;
    }

    uint8_t year = 0U;
    const uint8_t secondRegister = registers[0] & 0x7FU;
    const uint8_t minuteRegister = registers[1] & 0x7FU;
    if (!decodeBcd(secondRegister, value.second) ||
        !decodeBcd(minuteRegister, value.minute))
    {
        m_timeValid = false;
        return false;
    }

    const uint8_t hourRegister = registers[2];
    if ((hourRegister & 0x40U) != 0U)
    {
        uint8_t hour12 = 0U;
        if (!decodeBcd(hourRegister & 0x1FU, hour12) ||
            hour12 < 1U || hour12 > 12U)
        {
            m_timeValid = false;
            return false;
        }
        value.hour = static_cast<uint8_t>(hour12 % 12U);
        if ((hourRegister & 0x20U) != 0U)
            value.hour = static_cast<uint8_t>(value.hour + 12U);
    }
    else if (!decodeBcd(hourRegister & 0x3FU, value.hour))
    {
        m_timeValid = false;
        return false;
    }

    uint8_t weekday = 0U;
    if (!decodeBcd(registers[3] & 0x07U, weekday) ||
        weekday < 1U || weekday > 7U ||
        !decodeBcd(registers[4] & 0x3FU, value.day) ||
        (registers[5] & 0x80U) != 0U ||
        !decodeBcd(registers[5] & 0x1FU, value.month) ||
        !decodeBcd(registers[6], year))
    {
        m_timeValid = false;
        return false;
    }
    value.year = static_cast<uint16_t>(2000U + year);
    m_timeValid = validDateTime(value);
    return m_timeValid;
}

bool RtcClock::detected() const { return m_detected; }
bool RtcClock::timeValid() const { return m_timeValid; }

bool RtcClock::readRegisters(uint8_t start,
                             uint8_t* destination,
                             uint8_t count)
{
    if (m_wire == nullptr || destination == nullptr || count == 0U) return false;
    m_wire->beginTransmission(Address);
    if (m_wire->write(start) != 1U || m_wire->endTransmission(false) != 0U)
        return false;
    const uint8_t received = m_wire->requestFrom(Address, count);
    if (received != count) return false;
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (!m_wire->available()) return false;
        destination[i] = static_cast<uint8_t>(m_wire->read());
    }
    return true;
}
}
