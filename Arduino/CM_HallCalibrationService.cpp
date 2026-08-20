#include "CM_HallCalibrationService.h"

namespace CM
{

HallCalibrationResult::HallCalibrationResult()
    : valid(false),
      baselineAdc(0U),
      minAdc(0U),
      maxAdc(0U),
      recommendedThreshold(0U),
      recommendedHysteresis(0U),
      direction(HallCalibrationDirection::Rising),
      sampleCount(0U),
      durationMs(0UL)
{
}

HallCalibrationService::HallCalibrationService(HallTurnSource& hall)
    : m_hall(hall),
      m_state(HallCalibrationState::Idle),
      m_result(),
      m_hasResult(false),
      m_armedAtMs(0UL),
      m_startedAtMs(0UL),
      m_lastSampleMs(0UL),
      m_baselineSum(0UL),
      m_baselineSamples(0U),
      m_minAdc(1023U),
      m_maxAdc(0U),
      m_runSamples(0U)
{
}

bool HallCalibrationService::arm(uint32_t nowMs)
{
    if (m_state == HallCalibrationState::Running) return false;

    clearMeasurements();
    m_hall.reset(nowMs);
    m_state = HallCalibrationState::ArmedWaitingPhysicalStart;
    m_armedAtMs = nowMs;
    m_lastSampleMs = nowMs;
    return true;
}

bool HallCalibrationService::physicalStart(uint32_t nowMs)
{
    if (m_state != HallCalibrationState::ArmedWaitingPhysicalStart ||
        !baselineReady())
    {
        return false;
    }

    m_startedAtMs = nowMs;
    m_lastSampleMs = nowMs;
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
    m_hall.reset(nowMs);
    m_state = HallCalibrationState::Running;
    return true;
}

void HallCalibrationService::update(uint32_t nowMs, bool safeEnvironment)
{
    if (m_state == HallCalibrationState::Idle ||
        m_state == HallCalibrationState::Completed ||
        m_state == HallCalibrationState::Aborted)
    {
        return;
    }

    if (!safeEnvironment)
    {
        abort();
        return;
    }

    if (m_state == HallCalibrationState::ArmedWaitingPhysicalStart)
    {
        if (static_cast<uint32_t>(nowMs - m_armedAtMs) >= ArmedTimeoutMs)
        {
            abort();
            return;
        }

        sampleBaseline(nowMs);
        return;
    }

    if (static_cast<uint32_t>(nowMs - m_startedAtMs) > AbsoluteRunTimeoutMs)
    {
        abort();
        return;
    }

    sampleRunning(nowMs);
    if (static_cast<uint32_t>(nowMs - m_startedAtMs) >= RunDurationMs)
        finish(nowMs);
}

void HallCalibrationService::abort()
{
    m_state = HallCalibrationState::Aborted;
    m_hasResult = false;
}

void HallCalibrationService::reset()
{
    clearMeasurements();
    m_state = HallCalibrationState::Idle;
}

HallCalibrationState HallCalibrationService::state() const
{
    return m_state;
}

bool HallCalibrationService::active() const
{
    return m_state == HallCalibrationState::ArmedWaitingPhysicalStart ||
           m_state == HallCalibrationState::Running;
}

bool HallCalibrationService::motorPermit() const
{
    return m_state == HallCalibrationState::Running;
}

bool HallCalibrationService::baselineReady() const
{
    return m_baselineSamples >= MinimumBaselineSamples;
}

bool HallCalibrationService::takeResult(HallCalibrationResult& result)
{
    if (!m_hasResult) return false;
    result = m_result;
    m_hasResult = false;
    return true;
}

void HallCalibrationService::sampleBaseline(uint32_t nowMs)
{
    if (m_baselineSamples != 0U &&
        static_cast<uint32_t>(nowMs - m_lastSampleMs) < BaselineSampleIntervalMs)
    {
        return;
    }

    (void)m_hall.pollTurn(nowMs);
    const uint16_t raw = m_hall.rawValue();
    if (m_baselineSamples < 64U)
    {
        m_baselineSum += raw;
        ++m_baselineSamples;
    }
    m_lastSampleMs = nowMs;
}

void HallCalibrationService::sampleRunning(uint32_t nowMs)
{
    if (m_runSamples != 0U &&
        static_cast<uint32_t>(nowMs - m_lastSampleMs) < SampleIntervalMs)
    {
        return;
    }

    (void)m_hall.pollTurn(nowMs);
    const uint16_t raw = m_hall.rawValue();
    if (raw < m_minAdc) m_minAdc = raw;
    if (raw > m_maxAdc) m_maxAdc = raw;
    if (m_runSamples < 0xFFFFU) ++m_runSamples;
    m_lastSampleMs = nowMs;
}

void HallCalibrationService::finish(uint32_t nowMs)
{
    m_state = HallCalibrationState::Completed;
    m_result = HallCalibrationResult();
    m_result.sampleCount = m_runSamples;
    m_result.durationMs = static_cast<uint32_t>(nowMs - m_startedAtMs);

    if (!baselineReady() || m_runSamples == 0U || m_maxAdc < m_minAdc)
    {
        m_hasResult = true;
        return;
    }

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

    m_result.baselineAdc = baseline;
    m_result.minAdc = m_minAdc;
    m_result.maxAdc = m_maxAdc;
    m_result.direction = rising
                             ? HallCalibrationDirection::Rising
                             : HallCalibrationDirection::Falling;

    if (span < MinimumSignalSpan)
    {
        m_hasResult = true;
        return;
    }

    uint16_t threshold = rising
                             ? static_cast<uint16_t>(baseline + span / 2U)
                             : static_cast<uint16_t>(baseline - span / 2U);
    if (threshold == 0U) threshold = 1U;
    if (threshold > 1023U) threshold = 1023U;

    uint16_t hysteresis = static_cast<uint16_t>(span / 4U);
    if (hysteresis < 5U) hysteresis = 5U;
    if (hysteresis > 128U) hysteresis = 128U;
    if (hysteresis >= threshold)
        hysteresis = threshold > 1U ? static_cast<uint16_t>(threshold - 1U) : 1U;

    if (hysteresis == 0U || hysteresis >= threshold)
    {
        m_hasResult = true;
        return;
    }

    m_result.recommendedThreshold = threshold;
    m_result.recommendedHysteresis = hysteresis;
    m_result.valid = true;
    m_hasResult = true;
}

void HallCalibrationService::clearMeasurements()
{
    m_result = HallCalibrationResult();
    m_hasResult = false;
    m_armedAtMs = 0UL;
    m_startedAtMs = 0UL;
    m_lastSampleMs = 0UL;
    m_baselineSum = 0UL;
    m_baselineSamples = 0U;
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
}

} // namespace CM
