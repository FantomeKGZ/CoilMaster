#ifndef CM_PINS_H
#define CM_PINS_H

#include <Arduino.h>

namespace CM
{
namespace Pins
{
constexpr uint8_t KeypadRow0 = 2U;
constexpr uint8_t KeypadRow1 = 3U;
constexpr uint8_t KeypadRow2 = 4U;
constexpr uint8_t KeypadRow3 = 5U;
constexpr uint8_t KeypadCol0 = 6U;
constexpr uint8_t KeypadCol1 = 7U;
constexpr uint8_t KeypadCol2 = 8U;
constexpr uint8_t KeypadCol3 = 9U;
constexpr uint8_t StartButton = 10U;
constexpr uint8_t Buzzer = A3;
constexpr uint8_t Ssr = 12U;
constexpr uint8_t Hall = A0;

// SoftwareSerial: constructor order is RX, TX.
// Arduino A1 (TX) -> LLC -> ESP32 GPIO16 (RX2)
// Arduino A2 (RX) <- LLC <- ESP32 GPIO17 (TX2)
constexpr uint8_t EspTx = A1;
constexpr uint8_t EspRx = A2;
}

namespace Defaults
{
constexpr uint8_t LcdI2cAddress = 0x27U;
constexpr uint16_t HallThreshold = 590U;
constexpr uint16_t HallHysteresis = 50U;
constexpr uint16_t StartDebounceMs = 40U;
constexpr uint16_t SimulatedTurnIntervalMs = 250U;
constexpr uint32_t EspUartBaud = 9600UL;
}
}

#endif // CM_PINS_H
