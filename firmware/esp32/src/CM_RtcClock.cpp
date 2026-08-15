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

uint8_t encodeBcd(uint8_t value)
{
    return static_cast<uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

uint8_t weekdayFor(const RtcDateTime& value)
{
    static const uint8_t monthOffsets[] =
        {0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U};
    uint16_t year = value.year;
    if (value.month < 3U) --year;
    const uint16_t result = static_cast<uint16_t>(
        year + year / 4U - year / 100U + year / 400U +
        monthOffsets[value.month - 1U] + value.day);
    return static_cast<uint8_t>(result % 7U + 1U);
}

uint32_t secondsSince2000(const RtcDateTime& value)
{
    uint32_t days = 0UL;
    for (uint16_t year = 2000U; year < value.year; ++year)
        days += leapYear(year) ? 366UL : 365UL;
    static const uint8_t daysPerMonth[] =
        {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    for (uint8_t month = 1U; month < value.month; ++month)
    {
        days += daysPerMonth[month - 1U];
        if (month == 2U && leapYear(value.year)) ++days;
    }
    days += static_cast<uint32_t>(value.day - 1U);
    return days * 86400UL + static_cast<uint32_t>(value.hour) * 3600UL +
           static_cast<uint32_t>(value.minute) * 60UL + value.second;
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

bool RtcClock::set(const RtcDateTime& value)
{
    if (m_wire == nullptr || !validDateTime(value)) return false;
    const uint8_t registers[] = {
        encodeBcd(value.second),
        encodeBcd(value.minute),
        encodeBcd(value.hour),
        encodeBcd(weekdayFor(value)),
        encodeBcd(value.day),
        encodeBcd(value.month),
        encodeBcd(static_cast<uint8_t>(value.year - 2000U))
    };
    if (!writeRegisters(0x00U, registers,
                        static_cast<uint8_t>(sizeof(registers))))
        return false;

    uint8_t status = 0U;
    if (!readRegisters(0x0FU, &status, 1U)) return false;
    status = static_cast<uint8_t>(status & ~0x80U);
    if (!writeRegisters(0x0FU, &status, 1U)) return false;

    RtcDateTime verified;
    if (!read(verified)) return false;
    const uint32_t expectedSeconds = secondsSince2000(value);
    const uint32_t verifiedSeconds = secondsSince2000(verified);
    return verifiedSeconds >= expectedSeconds &&
           verifiedSeconds - expectedSeconds <= 2UL;
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


bool RtcClock::writeRegisters(uint8_t start,
                              const uint8_t* source,
                              uint8_t count)
{
    if (m_wire == nullptr || source == nullptr || count == 0U) return false;
    m_wire->beginTransmission(Address);
    if (m_wire->write(start) != 1U)
    {
        m_wire->endTransmission();
        return false;
    }
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (m_wire->write(source[i]) != 1U)
        {
            m_wire->endTransmission();
            return false;
        }
    }
    return m_wire->endTransmission() == 0U;
}
}
