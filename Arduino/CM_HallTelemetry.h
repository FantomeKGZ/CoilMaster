#ifndef CM_HALL_TELEMETRY_H
#define CM_HALL_TELEMETRY_H

#include <Arduino.h>

#include "CM_HallTurnSource.h"

namespace CM
{

struct HallTelemetrySnapshot
{
    bool valid;
    uint16_t rawAdc;
    uint16_t windowMin;
    uint16_t windowMax;
    uint16_t threshold;
    uint16_t hysteresis;
    uint16_t releaseBoundary;
    uint16_t releaseDebounceMs;
    bool inverted;
    bool magnetDetected;
    HallRearmState rearmState;
    uint16_t sampleCount;
    uint32_t capturedAtMs;

    HallTelemetrySnapshot();
};

class HallTelemetryService
{
public:
    static constexpr uint16_t DefaultSampleIntervalMs = 50U;

    HallTelemetryService(HallTurnSource& hall,
                         uint16_t sampleIntervalMs = DefaultSampleIntervalMs);

    void setEnabled(bool enabled, uint32_t nowMs);
    bool enabled() const;

    void update(uint32_t nowMs);
    bool takeSnapshot(HallTelemetrySnapshot& snapshot);
    void resetWindow(uint32_t nowMs);

private:
    void recordSample(uint32_t nowMs);

    HallTurnSource& m_hall;
    uint16_t m_sampleIntervalMs;
    uint32_t m_lastSampleMs;
    uint16_t m_windowMin;
    uint16_t m_windowMax;
    uint16_t m_sampleCount;
    bool m_enabled;
    bool m_hasSamples;
};

} // namespace CM

#endif // CM_HALL_TELEMETRY_H
