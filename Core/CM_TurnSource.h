#ifndef CM_TURN_SOURCE_H
#define CM_TURN_SOURCE_H

#include <stdint.h>

namespace CM
{

/** Hardware-independent source of winding-turn events. */
class ITurnSource
{
public:
    virtual ~ITurnSource() {}

    /**
     * Poll the source.
     * @return true exactly once for every newly detected or simulated turn.
     */
    virtual bool pollTurn(uint32_t nowMs) = 0;

    /** Reset internal edge/timing state before a new winding starts. */
    virtual void reset(uint32_t nowMs) = 0;
};

/**
 * Non-blocking software turn generator for tests without a motor.
 * It produces one turn event per configured interval while enabled.
 */
class SimulatedTurnSource : public ITurnSource
{
public:
    explicit SimulatedTurnSource(uint16_t intervalMs = 250U);

    void setEnabled(bool enabled, uint32_t nowMs);
    bool isEnabled() const;
    void setIntervalMs(uint16_t intervalMs);
    uint16_t intervalMs() const;

    bool pollTurn(uint32_t nowMs) override;
    void reset(uint32_t nowMs) override;

private:
    bool m_enabled;
    uint16_t m_intervalMs;
    uint32_t m_lastTurnMs;
};

} // namespace CM

#endif // CM_TURN_SOURCE_H
