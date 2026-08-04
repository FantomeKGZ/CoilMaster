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
constexpr uint8_t Buzzer = 11U;
constexpr uint8_t Ssr = 12U;
constexpr uint8_t Hall = A0;
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
}
}

#endif // CM_PINS_H
