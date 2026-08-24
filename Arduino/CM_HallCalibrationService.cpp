#include "CM_HallCalibrationService.h"

#include "CM_HallCalibrationRawBridge.h"

namespace CM
{

HallCalibrationResult::HallCalibrationResult()
    : measurementId(0UL)
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
      m_measurementId(0UL),
      m_baselineSamples(0U),
      m_runSamples(0U)
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
        !baselineReady() || m_runSamples == 0U || m_measurementId == 0UL)
    {
        return false;
    }

    result = HallCalibrationResult();
    result.measurementId = m_measurementId;
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
        const uint16_t sequence = m_baselineSamples;
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
    m_measurementId = measurementIdentity(nowMs);
    m_resultPending = baselineReady() && m_runSamples != 0U && m_measurementId != 0UL;
}

uint32_t HallCalibrationService::measurementIdentity(uint32_t completedAtMs) const
{
    uint32_t token = m_armedAtMs ^ m_startedAtMs ^ completedAtMs;
    token ^= (static_cast<uint32_t>(m_runSamples) << 16U) |
             static_cast<uint32_t>(m_runSamples);
    token ^= token << 13U;
    token ^= token >> 17U;
    token ^= token << 5U;
    return token == 0UL ? 1UL : token;
}

void HallCalibrationService::clearMeasurements()
{
    m_resultPending = false;
    m_armedAtMs = 0UL;
    m_lastPeerContactMs = 0UL;
    m_applyConfirmAtMs = 0UL;
    m_startedAtMs = 0UL;
    m_lastSampleMs = 0UL;
    m_measurementId = 0UL;
    m_baselineSamples = 0U;
    m_runSamples = 0U;
}

} // namespace CM
