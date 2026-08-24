#include "CM_HallCalibrationRawCollector.h"

namespace CM
{

HallCalibrationRawSummary::HallCalibrationRawSummary()
    : baselineAdc(0U), minAdc(0U), maxAdc(0U), baselineSamples(0U),
      runSamples(0U), durationMs(0UL), valid(false)
{
}

HallCalibrationRawCollector::HallCalibrationRawCollector()
{
    reset();
}

void HallCalibrationRawCollector::reset()
{
    m_baselineSum = 0UL;
    m_baselineSamples = 0U;
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
    m_durationMs = 0UL;
    m_running = false;
    m_finished = false;
}

bool HallCalibrationRawCollector::addBaselineSample(uint16_t rawAdc)
{
    if (rawAdc > 1023U || m_running || m_finished ||
        m_baselineSamples >= MaxBaselineSamples)
    {
        return false;
    }

    m_baselineSum += static_cast<uint32_t>(rawAdc);
    ++m_baselineSamples;
    return true;
}

void HallCalibrationRawCollector::beginRun()
{
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
    m_durationMs = 0UL;
    m_running = true;
    m_finished = false;
}

bool HallCalibrationRawCollector::addRunSample(uint16_t rawAdc)
{
    if (rawAdc > 1023U || !m_running || m_finished) return false;

    if (rawAdc < m_minAdc) m_minAdc = rawAdc;
    if (rawAdc > m_maxAdc) m_maxAdc = rawAdc;
    if (m_runSamples < 0xFFFFU) ++m_runSamples;
    return true;
}

bool HallCalibrationRawCollector::finish(uint32_t durationMs)
{
    if (!m_running || m_runSamples == 0U || durationMs == 0UL)
        return false;

    m_durationMs = durationMs;
    m_running = false;
    m_finished = true;
    return summary().valid;
}

HallCalibrationRawSummary HallCalibrationRawCollector::summary() const
{
    HallCalibrationRawSummary result;
    result.baselineSamples = m_baselineSamples;
    result.runSamples = m_runSamples;
    result.durationMs = m_durationMs;

    if (m_baselineSamples != 0U)
    {
        result.baselineAdc = static_cast<uint16_t>(
            m_baselineSum / static_cast<uint32_t>(m_baselineSamples));
    }

    if (m_runSamples != 0U)
    {
        result.minAdc = m_minAdc;
        result.maxAdc = m_maxAdc;
    }

    result.valid = m_finished &&
                   m_baselineSamples >= MinimumBaselineSamples &&
                   m_runSamples != 0U && m_durationMs != 0UL &&
                   m_maxAdc >= m_minAdc;
    return result;
}

} // namespace CM
