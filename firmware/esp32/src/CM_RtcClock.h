#ifndef CM_RTC_CLOCK_H
#define CM_RTC_CLOCK_H

#include <Arduino.h>
#include <Wire.h>

namespace CM
{
struct RtcDateTime
{
    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
};

class RtcClock
{
public:
    bool begin(int8_t sdaPin, int8_t sclPin);
    bool read(RtcDateTime& value);
    bool set(const RtcDateTime& value);

    // Configures ESP32 SNTP for Kyrgyzstan/Bishkek (UTC+6, no DST) and keeps
    // the external DS3231 aligned whenever Internet time becomes available.
    // The caller decides when writing the RTC is operationally safe.
    void beginNetworkSync();
    void updateNetworkSync(uint32_t nowMs,
                           bool networkConnected,
                           bool rtcWriteAllowed);

    // CoilMaster owns one production RTC instance. begin() registers that
    // instance so NetworkManager can drive synchronization without coupling
    // main.cpp to NTP details.
    static void updateActiveNetworkSync(uint32_t nowMs,
                                        bool networkConnected,
                                        bool rtcWriteAllowed);

    bool detected() const;
    bool timeValid() const;
    bool networkSyncConfigured() const;
    bool networkTimeSynced() const;
    uint32_t lastNetworkSyncMs() const;
    const char* lastNetworkSyncResult() const;
    const char* lastSetResult() const;
    const char* timezoneName() const;

private:
    static constexpr uint8_t Address = 0x68U;
    static constexpr uint32_t NetworkRetryMs = 30000UL;
    static constexpr uint32_t NetworkResyncMs = 21600000UL; // 6 hours.

    bool readRegisters(uint8_t start, uint8_t* destination, uint8_t count);
    bool writeRegisters(uint8_t start, const uint8_t* source, uint8_t count);
    bool networkDateTime(RtcDateTime& value) const;

    static RtcClock* s_activeClock;

    TwoWire* m_wire = nullptr;
    bool m_detected = false;
    bool m_timeValid = false;
    bool m_networkSyncConfigured = false;
    bool m_networkTimeSynced = false;
    uint32_t m_lastNetworkSyncMs = 0UL;
    uint32_t m_nextNetworkAttemptMs = 0UL;
    const char* m_lastNetworkSyncResult = "NOT_CONFIGURED";
    const char* m_lastSetResult = "NOT_ATTEMPTED";
};
}

#endif // CM_RTC_CLOCK_H
