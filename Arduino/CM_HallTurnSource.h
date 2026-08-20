#ifndef CM_HALL_TURN_SOURCE_H
#define CM_HALL_TURN_SOURCE_H

#include <Arduino.h>

#include "../Core/CM_TurnSource.h"

namespace CM
{

enum class HallRearmState : uint8_t
{
    Armed = 0U,
    WaitingRelease,
    ReleaseDebounce
};

/**
 * @brief Non-blocking SS49E adapter using threshold, hysteresis and release debounce.
 *
 * One turn is emitted when the configured active threshold is reached. Rising
 * mode treats ADC >= threshold as magnet present; falling mode treats ADC <=
 * threshold as magnet present. After detection the signal must cross the
 * opposite release boundary and remain there for the configured debounce
 * interval before another turn can be emitted. A single noisy ADC sample
 * therefore cannot re-arm the counter while a magnet is still hovering near
 * SS49E.
 */
class HallTurnSource : public ITurnSource
{
public:
    HallTurnSource(uint8_t analogPin,
                   uint16_t threshold = 590U,
                   uint16_t hysteresis = 50U,
                   uint16_t releaseDebounceMs = 25U,
                   bool inverted = false);

    bool pollTurn(uint32_t nowMs) override;
    void reset(uint32_t nowMs) override;

    void setThreshold(uint16_t threshold);
    uint16_t threshold() const;

    void setHysteresis(uint16_t hysteresis);
    uint16_t hysteresis() const;

    void setReleaseDebounceMs(uint16_t releaseDebounceMs);
    uint16_t releaseDebounceMs() const;

    void setInverted(bool inverted);
    bool inverted() const;

    uint16_t rawValue() const;
    bool magnetDetected() const;
    HallRearmState rearmState() const;
    uint16_t releaseBoundary() const;

private:
    uint16_t releaseThreshold() const;
    bool activeLevelReached() const;
    bool releaseLevelReached() const;

    uint8_t m_analogPin;
    uint16_t m_threshold;
    uint16_t m_hysteresis;
    uint16_t m_releaseDebounceMs;
    uint16_t m_rawValue;
    uint32_t m_releaseCandidateSinceMs;
    bool m_releaseCandidate;
    bool m_magnetDetected;
    bool m_inverted;
};

} // namespace CM

#endif // CM_HALL_TURN_SOURCE_H
