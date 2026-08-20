#include "CM_HallTelemetry.h"

namespace CM
{

HallTelemetrySnapshot::HallTelemetrySnapshot()
    : valid(false),
      rawAdc(0U),
      windowMin(0U),
      windowMax(0U),
      threshold(0U),
      hysteresis(0U),
      releaseBoundary(0U),
      releaseDebounceMs(0U),
      inverted(false),
      magnetDetected(false),
      rearmState(HallRearmState::Armed),
      sampleCount(0U),
      capturedAtMs(0UL)
{
}

HallTelemetryService::HallTelemetryService(HallTurnSource& hall,
                                           uint16_t sampleIntervalMs)
    : m_hall(hall),
      m_sampleIntervalMs(sampleIntervalMs == 0U ? 1U : sampleIntervalMs),
      m_lastSampleMs(0UL),
      m_windowMin(0U),
      m_windowMax(0U),
      m_sampleCount(0U),
      m_enabled(false),
      m_hasSamples(false)
{
}

void HallTelemetryService::setEnabled(bool enabled, uint32_t nowMs)
{
    if (enabled == m_enabled) return;

    m_enabled = enabled;
    resetWindow(nowMs);

    if (m_enabled)
    {
        // Sync the Hall latch with the physical level before diagnostic polling.
        // No WindingEvent is emitted; any returned turn from later diagnostic
        // samples is intentionally ignored by this service.
        m_hall.reset(nowMs);
    }
}

bool HallTelemetryService::enabled() const
{
    return m_enabled;
}

void HallTelemetryService::update(uint32_t nowMs)
{
    if (!m_enabled) return;

    if (!m_hasSamples ||
        static_cast<uint32_t>(nowMs - m_lastSampleMs) >=
            static_cast<uint32_t>(m_sampleIntervalMs))
    {
        recordSample(nowMs);
    }
}

bool HallTelemetryService::takeSnapshot(HallTelemetrySnapshot& snapshot)
{
    if (!m_enabled || !m_hasSamples || m_sampleCount == 0U) return false;

    snapshot = HallTelemetrySnapshot();
    snapshot.valid = true;
    snapshot.rawAdc = m_hall.rawValue();
    snapshot.windowMin = m_windowMin;
    snapshot.windowMax = m_windowMax;
    snapshot.threshold = m_hall.threshold();
    snapshot.hysteresis = m_hall.hysteresis();
    snapshot.releaseBoundary = m_hall.releaseBoundary();
    snapshot.releaseDebounceMs = m_hall.releaseDebounceMs();
    snapshot.inverted = m_hall.inverted();
    snapshot.magnetDetected = m_hall.magnetDetected();
    snapshot.rearmState = m_hall.rearmState();
    snapshot.sampleCount = m_sampleCount;
    snapshot.capturedAtMs = m_lastSampleMs;

    // Each consumer snapshot covers one bounded measurement window so min/max
    // remain useful and counters cannot grow indefinitely on an Uno.
    m_windowMin = snapshot.rawAdc;
    m_windowMax = snapshot.rawAdc;
    m_sampleCount = 0U;
    m_hasSamples = false;
    return true;
}

void HallTelemetryService::resetWindow(uint32_t nowMs)
{
    m_lastSampleMs = nowMs;
    m_windowMin = 0U;
    m_windowMax = 0U;
    m_sampleCount = 0U;
    m_hasSamples = false;
}

void HallTelemetryService::recordSample(uint32_t nowMs)
{
    // pollTurn updates the real Hall latch/re-arm state; telemetry intentionally
    // discards the returned turn because this service is diagnostic-only and is
    // enabled only outside the winding path.
    (void)m_hall.pollTurn(nowMs);

    const uint16_t raw = m_hall.rawValue();
    if (!m_hasSamples)
    {
        m_windowMin = raw;
        m_windowMax = raw;
        m_hasSamples = true;
    }
    else
    {
        if (raw < m_windowMin) m_windowMin = raw;
        if (raw > m_windowMax) m_windowMax = raw;
    }

    if (m_sampleCount < 0xFFFFU) ++m_sampleCount;
    m_lastSampleMs = nowMs;
}

} // namespace CM
