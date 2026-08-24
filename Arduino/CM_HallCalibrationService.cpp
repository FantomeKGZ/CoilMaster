#include "CM_HallCalibrationService.h"

#include "CM_HallCalibrationRawBridge.h"

namespace CM
{

HallCalibrationResult::HallCalibrationResult()
    : measurementId(0UL),
      baselineAdc(0U),
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
      m_applyConfirmAtMs(0UL),
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
    if (m_state == HallCalibrationState::Running ||
        m_state == HallCalibrationState::WaitingApplyConfirm)
    {
        return false;
    }

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

bool HallCalibrationService::beginApplyConfirm(uint32_t measurementId,
                                               uint32_t nowMs)
{
    if (m_state != HallCalibrationState::Completed || measurementId == 0UL)
        return false;

    HallCalibrationResult result;
    if (!populateResult(result) || result.measurementId != measurementId)
        return false;

    m_state = HallCalibrationState::WaitingApplyConfirm;
    m_applyConfirmAtMs = nowMs;
    m_lastPeerContactMs = nowMs;
    return true;
}

void HallCalibrationService::completeApply()
{
    if (m_state == HallCalibrationState::WaitingApplyConfirm)
    {
        m_state = HallCalibrationState::Completed;
        m_applyConfirmAtMs = 0UL;
    }
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

    if (m_state == HallCalibrationState::WaitingApplyConfirm)
    {
        if (static_cast<uint32_t>(nowMs - m_applyConfirmAtMs) >=
            ApplyConfirmTimeoutMs)
        {
            abort();
        }
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
    m_applyConfirmAtMs = 0UL;
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
           m_state == HallCalibrationState::Running ||
           m_state == HallCalibrationState::WaitingApplyConfirm;
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
    if ((m_state != HallCalibrationState::Completed &&
         m_state != HallCalibrationState::WaitingApplyConfirm) ||
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
    result.measurementId = measurementIdentity(result);
    return result.measurementId != 0UL;
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
        const uint16_t sequence = m_baselineSamples;
        m_baselineSum += raw;
        ++m_baselineSamples;
        (void)HallCalibrationRawBridge::publish(
            HallCalibrationSamplePhase::Baseline,
            raw,
            sequence,
            static_cast<uint32_t>(nowMs - m_armedAtMs));
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
    const uint16_t sequence = m_runSamples;
    if (raw < m_minAdc) m_minAdc = raw;
    if (raw > m_maxAdc) m_maxAdc = raw;
    if (m_runSamples < 0xFFFFU) ++m_runSamples;
    (void)HallCalibrationRawBridge::publish(
        HallCalibrationSamplePhase::Run,
        raw,
        sequence,
        static_cast<uint32_t>(nowMs - m_startedAtMs));
    m_lastSampleMs = nowMs;
}

void HallCalibrationService::finish(uint32_t nowMs)
{
    m_state = HallCalibrationState::Completed;
    m_resultDurationMs = static_cast<uint32_t>(nowMs - m_startedAtMs);
    m_resultPending = baselineReady() && m_runSamples != 0U && m_maxAdc >= m_minAdc;
}

uint32_t HallCalibrationService::measurementIdentity(
    const HallCalibrationResult& result) const
{
    // Transient correlation token only: bind a proposal to this exact arm/result
    // without carrying a heavy general-purpose hash loop in the Uno image.
    uint32_t hash = m_armedAtMs ^ result.durationMs;
    hash ^= static_cast<uint32_t>(result.baselineAdc) << 22U;
    hash ^= static_cast<uint32_t>(result.minAdc) << 12U;
    hash ^= static_cast<uint32_t>(result.maxAdc) << 2U;
    hash ^= (static_cast<uint32_t>(result.sampleCount) << 16U) |
            static_cast<uint32_t>(result.sampleCount);
    hash ^= hash << 13U;
    hash ^= hash >> 17U;
    hash ^= hash << 5U;
    return hash == 0UL ? 1UL : hash;
}

void HallCalibrationService::clearMeasurements()
{
    m_resultPending = false;
    m_armedAtMs = 0UL;
    m_lastPeerContactMs = 0UL;
    m_applyConfirmAtMs = 0UL;
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
