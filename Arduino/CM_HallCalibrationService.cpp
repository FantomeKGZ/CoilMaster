#include "CM_HallCalibrationService.h"

namespace CM
{

HallCalibrationResult::HallCalibrationResult()
    : baselineAdc(0U),
      minAdc(0U),
      maxAdc(0U),
      sampleCount(0U),
      durationMs(0UL)
{
}

HallCalibrationService::HallCalibrationService(HallTurnSource& hall)
    : m_hall(hall),
      m_state(HallCalibrationState::Idle),
      m_resultPending(false),
      m_armedAtMs(0UL),
      m_lastPeerContactMs(0UL),
      m_startedAtMs(0UL),
      m_lastSampleMs(0UL),
      m_baselineSum(0UL),
      m_baselineSamples(0U),
      m_minAdc(1023U),
      m_maxAdc(0U),
      m_runSamples(0U),
      m_resultDurationMs(0UL)
{
}

bool HallCalibrationService::arm(uint32_t nowMs)
{
    if (m_state == HallCalibrationState::Running) return false;

    clearMeasurements();
    m_hall.reset(nowMs);
    m_state = HallCalibrationState::WaitingLocalConfirm;
    m_armedAtMs = nowMs;
    m_lastPeerContactMs = nowMs;
    m_lastSampleMs = nowMs;
    return true;
}

void HallCalibrationService::notePeerContact(uint32_t nowMs)
{
    if (active()) m_lastPeerContactMs = nowMs;
}

bool HallCalibrationService::confirmLocal(uint32_t nowMs)
{
    if (m_state != HallCalibrationState::WaitingLocalConfirm) return false;

    m_state = HallCalibrationState::ArmedWaitingPhysicalStart;
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

    if (!safeEnvironment ||
        static_cast<uint32_t>(nowMs - m_lastPeerContactMs) >= PeerTimeoutMs)
    {
        abort();
        return;
    }

    if (m_state == HallCalibrationState::WaitingLocalConfirm ||
        m_state == HallCalibrationState::ArmedWaitingPhysicalStart)
    {
        if (static_cast<uint32_t>(nowMs - m_armedAtMs) >= ArmedTimeoutMs)
        {
            abort();
            return;
        }

        if (m_state == HallCalibrationState::ArmedWaitingPhysicalStart)
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
    m_resultPending = false;
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
    return m_state == HallCalibrationState::WaitingLocalConfirm ||
           m_state == HallCalibrationState::ArmedWaitingPhysicalStart ||
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

bool HallCalibrationService::populateResult(HallCalibrationResult& result) const
{
    if (m_state != HallCalibrationState::Completed ||
        !baselineReady() || m_runSamples == 0U || m_maxAdc < m_minAdc)
    {
        return false;
    }

    result = HallCalibrationResult();
    result.baselineAdc = static_cast<uint16_t>(
        m_baselineSum / static_cast<uint32_t>(m_baselineSamples));
    result.minAdc = m_minAdc;
    result.maxAdc = m_maxAdc;
    result.sampleCount = m_runSamples;
    result.durationMs = m_resultDurationMs;
    return true;
}

bool HallCalibrationService::takeResult(HallCalibrationResult& result)
{
    if (!m_resultPending) return false;
    m_resultPending = false;
    return populateResult(result);
}

bool HallCalibrationService::latestResult(HallCalibrationResult& result) const
{
    return populateResult(result);
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
    m_resultDurationMs = static_cast<uint32_t>(nowMs - m_startedAtMs);
    m_resultPending = baselineReady() && m_runSamples != 0U && m_maxAdc >= m_minAdc;
}

void HallCalibrationService::clearMeasurements()
{
    m_resultPending = false;
    m_armedAtMs = 0UL;
    m_lastPeerContactMs = 0UL;
    m_startedAtMs = 0UL;
    m_lastSampleMs = 0UL;
    m_baselineSum = 0UL;
    m_baselineSamples = 0U;
    m_minAdc = 1023U;
    m_maxAdc = 0U;
    m_runSamples = 0U;
    m_resultDurationMs = 0UL;
}

} // namespace CM
