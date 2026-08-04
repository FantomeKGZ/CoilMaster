#include "CM_TurnSource.h"

namespace CM
{

SimulatedTurnSource::SimulatedTurnSource(uint16_t intervalMs)
    : m_enabled(false),
      m_intervalMs(intervalMs == 0U ? 1U : intervalMs),
      m_lastTurnMs(0UL)
{
}

void SimulatedTurnSource::setEnabled(bool enabled, uint32_t nowMs)
{
    m_enabled = enabled;
    m_lastTurnMs = nowMs;
}

bool SimulatedTurnSource::isEnabled() const
{
    return m_enabled;
}

void SimulatedTurnSource::setIntervalMs(uint16_t intervalMs)
{
    m_intervalMs = intervalMs == 0U ? 1U : intervalMs;
}

uint16_t SimulatedTurnSource::intervalMs() const
{
    return m_intervalMs;
}

bool SimulatedTurnSource::pollTurn(uint32_t nowMs)
{
    if (!m_enabled)
    {
        return false;
    }

    if (static_cast<uint32_t>(nowMs - m_lastTurnMs) < m_intervalMs)
    {
        return false;
    }

    m_lastTurnMs = nowMs;
    return true;
}

void SimulatedTurnSource::reset(uint32_t nowMs)
{
    m_lastTurnMs = nowMs;
}

} // namespace CM
