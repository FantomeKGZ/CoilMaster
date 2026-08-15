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
    bool detected() const;
    bool timeValid() const;

private:
    static constexpr uint8_t Address = 0x68U;

    bool readRegisters(uint8_t start, uint8_t* destination, uint8_t count);

    TwoWire* m_wire = nullptr;
    bool m_detected = false;
    bool m_timeValid = false;
};
}

#endif // CM_RTC_CLOCK_H
