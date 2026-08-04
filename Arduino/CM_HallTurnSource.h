#ifndef CM_HALL_TURN_SOURCE_H
#define CM_HALL_TURN_SOURCE_H

#include <Arduino.h>

#include "../Core/CM_TurnSource.h"

namespace CM
{

/**
 * @brief Non-blocking SS49E adapter using threshold and hysteresis.
 *
 * One turn is emitted on the rising transition above the configured threshold.
 * The sensor must fall below threshold - hysteresis before another turn can be
 * emitted. This prevents repeated counts while the magnet remains near SS49E.
 */
class HallTurnSource : public ITurnSource
{
public:
    HallTurnSource(uint8_t analogPin,
                   uint16_t threshold = 590U,
                   uint16_t hysteresis = 50U);

    bool pollTurn(uint32_t nowMs) override;
    void reset(uint32_t nowMs) override;

    void setThreshold(uint16_t threshold);
    uint16_t threshold() const;

    void setHysteresis(uint16_t hysteresis);
    uint16_t hysteresis() const;

    uint16_t rawValue() const;
    bool magnetDetected() const;

private:
    uint16_t releaseThreshold() const;

    uint8_t m_analogPin;
    uint16_t m_threshold;
    uint16_t m_hysteresis;
    uint16_t m_rawValue;
    bool m_magnetDetected;
};

} // namespace CM

#endif // CM_HALL_TURN_SOURCE_H
