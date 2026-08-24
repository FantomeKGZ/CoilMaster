#include "CM_HallCalibrationAnalyzer.h"

namespace CM
{

HallCalibrationProposal::HallCalibrationProposal()
    : valid(false),
      baselineAdc(0U),
      minAdc(0U),
      maxAdc(0U),
      recommendedThreshold(0U),
      recommendedHysteresis(0U),
      direction(HallSignalDirectionRemote::Rising),
      sampleCount(0U),
      durationMs(0UL)
{
}

HallCalibrationAnalyzer::HallCalibrationAnalyzer()
{
    reset();
}

void HallCalibrationAnalyzer::reset()
{
    m_baselineSum = 0UL;
    m_baselineSamples = 0U;
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
}

bool HallCalibrationAnalyzer::addBaselineSample(uint16_t rawAdc)
{
    if (rawAdc > 1023U || m_baselineSamples >= MaximumBaselineSamples)
        return false;

    m_baselineSum += rawAdc;
    ++m_baselineSamples;
    return true;
}

bool HallCalibrationAnalyzer::baselineReady() const
{
    return m_baselineSamples >= MinimumBaselineSamples;
}

void HallCalibrationAnalyzer::beginRun()
{
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
}

void HallCalibrationAnalyzer::addRunSample(uint16_t rawAdc)
{
    if (rawAdc > 1023U) return;
    if (rawAdc < m_minAdc) m_minAdc = rawAdc;
    if (rawAdc > m_maxAdc) m_maxAdc = rawAdc;
    if (m_runSamples < 0xFFFFU) ++m_runSamples;
}

HallCalibrationProposal HallCalibrationAnalyzer::finish(uint32_t durationMs) const
{
    HallCalibrationProposal proposal;
    proposal.sampleCount = m_runSamples;
    proposal.durationMs = durationMs;

    if (!baselineReady() || m_runSamples == 0U || m_maxAdc < m_minAdc)
        return proposal;

    const uint16_t baseline = static_cast<uint16_t>(
        m_baselineSum / static_cast<uint32_t>(m_baselineSamples));
    const uint16_t upward = m_maxAdc > baseline
                                ? static_cast<uint16_t>(m_maxAdc - baseline)
                                : 0U;
    const uint16_t downward = baseline > m_minAdc
                                  ? static_cast<uint16_t>(baseline - m_minAdc)
                                  : 0U;
    const bool rising = upward >= downward;
    const uint16_t span = rising ? upward : downward;

    proposal.baselineAdc = baseline;
    proposal.minAdc = m_minAdc;
    proposal.maxAdc = m_maxAdc;
    proposal.direction = rising
                             ? HallSignalDirectionRemote::Rising
                             : HallSignalDirectionRemote::Falling;

    if (span < MinimumSignalSpan) return proposal;

    uint16_t threshold = rising
                             ? static_cast<uint16_t>(baseline + span / 2U)
                             : static_cast<uint16_t>(baseline - span / 2U);
    if (threshold == 0U) threshold = 1U;
    if (threshold > 1023U) threshold = 1023U;

    uint16_t hysteresis = static_cast<uint16_t>(span / 4U);
    if (hysteresis < 5U) hysteresis = 5U;
    if (hysteresis > 128U) hysteresis = 128U;
    if (hysteresis >= threshold)
        hysteresis = threshold > 1U
                         ? static_cast<uint16_t>(threshold - 1U)
                         : 1U;

    if (hysteresis == 0U || hysteresis >= threshold) return proposal;

    proposal.recommendedThreshold = threshold;
    proposal.recommendedHysteresis = hysteresis;
    proposal.valid = true;
    return proposal;
}

} // namespace CM
