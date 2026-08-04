#include "CM_HallTurnSource.h"

namespace CM
{

HallTurnSource::HallTurnSource(uint8_t analogPin,
                               uint16_t threshold,
                               uint16_t hysteresis)
    : m_analogPin(analogPin),
      m_threshold(threshold),
      m_hysteresis(hysteresis),
      m_rawValue(0U),
      m_magnetDetected(false)
{
}

bool HallTurnSource::pollTurn(uint32_t nowMs)
{
    (void)nowMs;
    m_rawValue = static_cast<uint16_t>(analogRead(m_analogPin));

    if (!m_magnetDetected && m_rawValue >= m_threshold)
    {
        m_magnetDetected = true;
        return true;
    }

    if (m_magnetDetected && m_rawValue <= releaseThreshold())
    {
        m_magnetDetected = false;
    }

    return false;
}

void HallTurnSource::reset(uint32_t nowMs)
{
    (void)nowMs;
    m_rawValue = static_cast<uint16_t>(analogRead(m_analogPin));
    m_magnetDetected = m_rawValue >= m_threshold;
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
