#include "CM_HallCalibrationService.h"

#include "CM_HallCalibrationRawBridge.h"

namespace CM
{

#if CM_LCD_RU_EN
HallCalibrationState HallCalibrationService::s_displayState = HallCalibrationState::Idle;
uint32_t HallCalibrationService::s_displayStartedAtMs = 0UL;
#endif

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
      m_lastPublishMs(0UL),
      m_measurementId(0UL),
      m_runWindowMin(1023U),
      m_runWindowMax(0U),
      m_baselineSamples(0U),
      m_runSamples(0U)
{
#if CM_LCD_RU_EN
    s_displayState = HallCalibrationState::Idle;
    s_displayStartedAtMs = 0UL;
#endif
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
    // Keep the first protocol reply backward-compatible with the existing
    // ESP32 ARM handshake. update() promotes this automatically on the next
    // loop; no keypad confirmation is required from the operator.
    m_state = HallCalibrationState::WaitingLocalConfirm;
#if CM_LCD_RU_EN
    s_displayState = HallCalibrationState::ArmedWaitingPhysicalStart;
#endif
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
#if CM_LCD_RU_EN
    s_displayState = m_state;
#endif
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
    m_lastPublishMs = nowMs;
    m_runWindowMin = 1023U;
    m_runWindowMax = 0U;
    m_runSamples = 0U;
    m_hall.reset(nowMs);
    m_state = HallCalibrationState::Running;
#if CM_LCD_RU_EN
    s_displayState = m_state;
    s_displayStartedAtMs = nowMs;
#endif
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
#if CM_LCD_RU_EN
    s_displayState = m_state;
#endif
    m_applyConfirmAtMs = nowMs;
    m_lastPeerContactMs = nowMs;
    return true;
}

void HallCalibrationService::completeApply()
{
    if (m_state == HallCalibrationState::WaitingApplyConfirm)
    {
        m_state = HallCalibrationState::Completed;
#if CM_LCD_RU_EN
        s_displayState = m_state;
#endif
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

    if (m_state == HallCalibrationState::WaitingLocalConfirm)
    {
        (void)confirmLocal(nowMs);
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
#if CM_LCD_RU_EN
    s_displayState = m_state;
#endif
    m_resultPending = false;
    m_applyConfirmAtMs = 0UL;
}

void HallCalibrationService::reset()
{
    clearMeasurements();
    m_state = HallCalibrationState::Idle;
#if CM_LCD_RU_EN
    s_displayState = m_state;
#endif
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

#if CM_LCD_RU_EN
HallCalibrationState HallCalibrationService::displayState()
{
    return s_displayState;
}

uint32_t HallCalibrationService::displayStartedAtMs()
{
    return s_displayStartedAtMs;
}
#endif

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
    if (static_cast<uint32_t>(nowMs - m_lastSampleMs) < SampleIntervalMs)
        return;

    (void)m_hall.pollTurn(nowMs);
    const uint16_t raw = m_hall.rawValue();
    if (raw < m_runWindowMin) m_runWindowMin = raw;
    if (raw > m_runWindowMax) m_runWindowMax = raw;
    m_lastSampleMs = nowMs;

    if (static_cast<uint32_t>(nowMs - m_lastPublishMs) >= RunPublishIntervalMs)
        publishRunWindow(nowMs);
}

void HallCalibrationService::publishRunWindow(uint32_t nowMs)
{
    if (m_runWindowMin > m_runWindowMax) return;

    const uint32_t elapsedMs = static_cast<uint32_t>(nowMs - m_startedAtMs);
    if (m_runSamples < 0xFFFFU)
    {
        (void)HallCalibrationRawBridge::publish(
            HallCalibrationSamplePhase::Run,
            m_runWindowMin,
            m_runSamples,
            elapsedMs);
        ++m_runSamples;
    }

    if (m_runWindowMax != m_runWindowMin && m_runSamples < 0xFFFFU)
    {
        (void)HallCalibrationRawBridge::publish(
            HallCalibrationSamplePhase::Run,
            m_runWindowMax,
            m_runSamples,
            elapsedMs);
        ++m_runSamples;
    }

    m_runWindowMin = 1023U;
    m_runWindowMax = 0U;
    m_lastPublishMs = nowMs;
}

void HallCalibrationService::finish(uint32_t nowMs)
{
    publishRunWindow(nowMs);
    m_state = HallCalibrationState::Completed;
#if CM_LCD_RU_EN
    s_displayState = m_state;
#endif
    m_measurementId = measurementIdentity(nowMs);
    m_resultPending = baselineReady() && m_runSamples != 0U && m_measurementId != 0UL;
}

uint32_t HallCalibrationService::measurementIdentity(uint32_t completedAtMs) const
{
    // Exact transient correlation only, not authentication. A completed test
    // cannot share its completion tick with another active test; proposals are
    // RAM-only and expire/abort before a later calibration can reuse the gate.
    return completedAtMs == 0UL ? 1UL : completedAtMs;
}

void HallCalibrationService::clearMeasurements()
{
    m_resultPending = false;
    m_armedAtMs = 0UL;
    m_lastPeerContactMs = 0UL;
    m_applyConfirmAtMs = 0UL;
    m_startedAtMs = 0UL;
#if CM_LCD_RU_EN
    s_displayStartedAtMs = 0UL;
#endif
    m_lastSampleMs = 0UL;
    m_lastPublishMs = 0UL;
    m_measurementId = 0UL;
    m_runWindowMin = 1023U;
    m_runWindowMax = 0U;
    m_baselineSamples = 0U;
    m_runSamples = 0U;
}

} // namespace CM
