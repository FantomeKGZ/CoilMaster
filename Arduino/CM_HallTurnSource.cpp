#include "CM_HallTurnSource.h"

namespace CM
{

HallTurnSource::HallTurnSource(uint8_t analogPin,
                               uint16_t threshold,
                               uint16_t hysteresis,
                               uint16_t releaseDebounceMs)
    : m_analogPin(analogPin),
      m_threshold(threshold),
      m_hysteresis(hysteresis),
      m_releaseDebounceMs(releaseDebounceMs),
      m_rawValue(0U),
      m_releaseCandidateSinceMs(0UL),
      m_releaseCandidate(false),
      m_magnetDetected(false)
{
}

bool HallTurnSource::pollTurn(uint32_t nowMs)
{
    m_rawValue = static_cast<uint16_t>(analogRead(m_analogPin));

    if (!m_magnetDetected)
    {
        m_releaseCandidate = false;
        if (m_rawValue >= m_threshold)
        {
            m_magnetDetected = true;
            return true;
        }
        return false;
    }

    if (m_rawValue > releaseThreshold())
    {
        // Magnet is still present (or ADC bounced back into the active band).
        // Any pending release must start over so noise cannot re-arm counting.
        m_releaseCandidate = false;
        return false;
    }

    if (!m_releaseCandidate)
    {
        m_releaseCandidate = true;
        m_releaseCandidateSinceMs = nowMs;
        return false;
    }

    if (static_cast<uint32_t>(nowMs - m_releaseCandidateSinceMs) >=
        static_cast<uint32_t>(m_releaseDebounceMs))
    {
        m_magnetDetected = false;
        m_releaseCandidate = false;
    }

    return false;
}

void HallTurnSource::reset(uint32_t nowMs)
{
    m_rawValue = static_cast<uint16_t>(analogRead(m_analogPin));
    m_magnetDetected = m_rawValue >= m_threshold;
    m_releaseCandidate = false;
    m_releaseCandidateSinceMs = nowMs;
}

void HallTurnSource::setThreshold(uint16_t threshold)
{
    m_threshold = threshold > 1023U ? 1023U : threshold;
}

uint16_t HallTurnSource::threshold() const
{
    return m_threshold;
}

void HallTurnSource::setHysteresis(uint16_t hysteresis)
{
    m_hysteresis = hysteresis > 1023U ? 1023U : hysteresis;
}

uint16_t HallTurnSource::hysteresis() const
{
    return m_hysteresis;
}

void HallTurnSource::setReleaseDebounceMs(uint16_t releaseDebounceMs)
{
    m_releaseDebounceMs = releaseDebounceMs;
}

uint16_t HallTurnSource::releaseDebounceMs() const
{
    return m_releaseDebounceMs;
}

uint16_t HallTurnSource::rawValue() const
{
    return m_rawValue;
}

bool HallTurnSource::magnetDetected() const
{
    return m_magnetDetected;
}

uint16_t HallTurnSource::releaseThreshold() const
{
    return m_threshold > m_hysteresis
               ? static_cast<uint16_t>(m_threshold - m_hysteresis)
               : 0U;
}

} // namespace CM
