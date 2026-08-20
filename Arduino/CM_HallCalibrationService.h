#ifndef CM_HALL_CALIBRATION_SERVICE_H
#define CM_HALL_CALIBRATION_SERVICE_H

#include <Arduino.h>

#include "CM_HallTurnSource.h"

namespace CM
{

enum class HallCalibrationState : uint8_t
{
    Idle = 0U,
    ArmedWaitingPhysicalStart,
    Running,
    Completed,
    Aborted
};

enum class HallCalibrationDirection : uint8_t
{
    Rising = 0U,
    Falling
};

struct HallCalibrationResult
{
    bool valid;
    uint16_t baselineAdc;
    uint16_t minAdc;
    uint16_t maxAdc;
    uint16_t recommendedThreshold;
    uint16_t recommendedHysteresis;
    HallCalibrationDirection direction;
    uint16_t sampleCount;
    uint32_t durationMs;

    HallCalibrationResult();
};

class HallCalibrationService
{
public:
    explicit HallCalibrationService(HallTurnSource& hall);

    bool arm(uint32_t nowMs);
    bool physicalStart(uint32_t nowMs);
    void update(uint32_t nowMs, bool safeEnvironment);
    void abort();
    void reset();

    HallCalibrationState state() const;
    bool active() const;
    bool motorPermit() const;
    bool baselineReady() const;
    bool takeResult(HallCalibrationResult& result);

private:
    static constexpr uint16_t SampleIntervalMs = 10U;
    static constexpr uint16_t BaselineSampleIntervalMs = 20U;
    static constexpr uint8_t MinimumBaselineSamples = 8U;
    static constexpr uint16_t MinimumSignalSpan = 60U;
    static constexpr uint32_t ArmedTimeoutMs = 60000UL;
    static constexpr uint32_t RunDurationMs = 5000UL;
    static constexpr uint32_t AbsoluteRunTimeoutMs = 6500UL;

    void sampleBaseline(uint32_t nowMs);
    void sampleRunning(uint32_t nowMs);
    void finish(uint32_t nowMs);
    void clearMeasurements();

    HallTurnSource& m_hall;
    HallCalibrationState m_state;
    HallCalibrationResult m_result;
    bool m_hasResult;
    uint32_t m_armedAtMs;
    uint32_t m_startedAtMs;
    uint32_t m_lastSampleMs;
    uint32_t m_baselineSum;
    uint16_t m_baselineSamples;
    uint16_t m_minAdc;
    uint16_t m_maxAdc;
    uint16_t m_runSamples;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_SERVICE_H
