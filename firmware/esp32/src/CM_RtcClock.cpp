#include "CM_RtcClock.h"

#include <time.h>

namespace CM
{
RtcClock* RtcClock::s_activeClock = nullptr;

namespace
{
constexpr long BishkekUtcOffsetSeconds = 6L * 60L * 60L;
constexpr int BishkekDaylightOffsetSeconds = 0;
constexpr char BishkekTimezoneName[] = "Asia/Bishkek";

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
    s_activeClock = this;
    m_wire = &Wire;
    m_wire->begin(sdaPin, sclPin);
    RtcDateTime ignored;
    read(ignored);
    beginNetworkSync();
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
    if (m_wire == nullptr)
    {
        m_lastSetResult = "NOT_INITIALIZED";
        return false;
    }
    if (!validDateTime(value))
    {
        m_lastSetResult = "INVALID_DATETIME";
        return false;
    }

    const uint8_t registers[] = {
        encodeBcd(value.second),
        encodeBcd(value.minute),
        encodeBcd(value.hour),
        encodeBcd(weekdayFor(value)),
        encodeBcd(value.day),
        encodeBcd(value.month),
        encodeBcd(static_cast<uint8_t>(value.year - 2000U))
    };

    // A short retry covers transient I2C/NACK conditions without turning the
    // HTTP request into a long blocking operation.
    for (uint8_t attempt = 0U; attempt < 2U; ++attempt)
    {
        if (!writeRegisters(0x00U, registers,
                            static_cast<uint8_t>(sizeof(registers))))
        {
            m_lastSetResult = "TIME_REGISTER_WRITE_FAILED";
            delay(2U);
            continue;
        }
        m_detected = true;

        // OSF only needs a write when it is actually set. The previous code
        // rewrote the status register on every manual set, creating an extra
        // failure point even for a healthy clock.
        uint8_t status = 0U;
        if (!readRegisters(0x0FU, &status, 1U))
        {
            m_lastSetResult = "STATUS_READ_FAILED";
            delay(2U);
            continue;
        }
        if ((status & 0x80U) != 0U)
        {
            const uint8_t clearedStatus = static_cast<uint8_t>(status & ~0x80U);
            if (!writeRegisters(0x0FU, &clearedStatus, 1U))
            {
                m_lastSetResult = "OSF_CLEAR_FAILED";
                delay(2U);
                continue;
            }
        }

        RtcDateTime verified;
        if (!read(verified))
        {
            m_lastSetResult = "VERIFY_READ_FAILED";
            delay(2U);
            continue;
        }

        const uint32_t expectedSeconds = secondsSince2000(value);
        const uint32_t verifiedSeconds = secondsSince2000(verified);
        const uint32_t difference = expectedSeconds > verifiedSeconds
            ? expectedSeconds - verifiedSeconds
            : verifiedSeconds - expectedSeconds;
        if (difference <= 2UL)
        {
            m_timeValid = true;
            m_lastSetResult = "OK";
            return true;
        }

        m_lastSetResult = "VERIFY_TIME_MISMATCH";
        delay(2U);
    }

    Serial.print(F("RTC set failed: "));
    Serial.println(m_lastSetResult);
    return false;
}

void RtcClock::beginNetworkSync()
{
    if (m_networkSyncConfigured) return;
    configTime(BishkekUtcOffsetSeconds,
               BishkekDaylightOffsetSeconds,
               "pool.ntp.org",
               "time.google.com",
               "time.cloudflare.com");
    m_networkSyncConfigured = true;
    m_lastNetworkSyncResult = "WAITING_NETWORK_TIME";
    m_nextNetworkAttemptMs = 0UL;
}

void RtcClock::updateNetworkSync(uint32_t nowMs,
                                 bool networkConnected,
                                 bool rtcWriteAllowed)
{
    if (!m_networkSyncConfigured) beginNetworkSync();

    if (!networkConnected)
    {
        if (!m_networkTimeSynced)
            m_lastNetworkSyncResult = "WAITING_NETWORK";
        return;
    }

    if (!rtcWriteAllowed)
    {
        if (!m_networkTimeSynced)
            m_lastNetworkSyncResult = "WAITING_SAFE_IDLE";
        return;
    }

    if (m_networkTimeSynced &&
        static_cast<uint32_t>(nowMs - m_lastNetworkSyncMs) < NetworkResyncMs)
        return;

    if (m_nextNetworkAttemptMs != 0UL &&
        static_cast<int32_t>(nowMs - m_nextNetworkAttemptMs) < 0)
        return;

    RtcDateTime networkValue;
    if (!networkDateTime(networkValue))
    {
        m_lastNetworkSyncResult = "WAITING_NTP";
        m_nextNetworkAttemptMs = nowMs + NetworkRetryMs;
        return;
    }

    if (!set(networkValue))
    {
        m_lastNetworkSyncResult = m_lastSetResult;
        m_nextNetworkAttemptMs = nowMs + NetworkRetryMs;
        return;
    }

    m_networkTimeSynced = true;
    m_lastNetworkSyncMs = nowMs;
    m_nextNetworkAttemptMs = nowMs + NetworkResyncMs;
    m_lastNetworkSyncResult = "SYNCED";

    Serial.print(F("RTC NTP sync OK: Asia/Bishkek "));
    Serial.print(networkValue.year); Serial.print('-');
    if (networkValue.month < 10U) Serial.print('0');
    Serial.print(networkValue.month); Serial.print('-');
    if (networkValue.day < 10U) Serial.print('0');
    Serial.print(networkValue.day); Serial.print(' ');
    if (networkValue.hour < 10U) Serial.print('0');
    Serial.print(networkValue.hour); Serial.print(':');
    if (networkValue.minute < 10U) Serial.print('0');
    Serial.print(networkValue.minute); Serial.print(':');
    if (networkValue.second < 10U) Serial.print('0');
    Serial.println(networkValue.second);
}

void RtcClock::updateActiveNetworkSync(uint32_t nowMs,
                                       bool networkConnected,
                                       bool rtcWriteAllowed)
{
    if (s_activeClock == nullptr) return;
    s_activeClock->updateNetworkSync(nowMs, networkConnected, rtcWriteAllowed);
}

bool RtcClock::networkDateTime(RtcDateTime& value) const
{
    value = RtcDateTime();
    struct tm localTime = {};
    if (!getLocalTime(&localTime, 20UL)) return false;

    const int year = localTime.tm_year + 1900;
    const int month = localTime.tm_mon + 1;
    if (year < 2000 || year > 2099 ||
        month < 1 || month > 12 ||
        localTime.tm_mday < 1 || localTime.tm_mday > 31 ||
        localTime.tm_hour < 0 || localTime.tm_hour > 23 ||
        localTime.tm_min < 0 || localTime.tm_min > 59 ||
        localTime.tm_sec < 0 || localTime.tm_sec > 60)
        return false;

    value.year = static_cast<uint16_t>(year);
    value.month = static_cast<uint8_t>(month);
    value.day = static_cast<uint8_t>(localTime.tm_mday);
    value.hour = static_cast<uint8_t>(localTime.tm_hour);
    value.minute = static_cast<uint8_t>(localTime.tm_min);
    value.second = static_cast<uint8_t>(localTime.tm_sec == 60 ? 59 : localTime.tm_sec);
    return validDateTime(value);
}

bool RtcClock::detected() const { return m_detected; }
bool RtcClock::timeValid() const { return m_timeValid; }
bool RtcClock::networkSyncConfigured() const { return m_networkSyncConfigured; }
bool RtcClock::networkTimeSynced() const { return m_networkTimeSynced; }
uint32_t RtcClock::lastNetworkSyncMs() const { return m_lastNetworkSyncMs; }
const char* RtcClock::lastNetworkSyncResult() const
{
    return m_lastNetworkSyncResult;
}
const char* RtcClock::lastSetResult() const { return m_lastSetResult; }
const char* RtcClock::timezoneName() const { return BishkekTimezoneName; }

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
