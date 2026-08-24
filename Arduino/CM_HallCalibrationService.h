#ifndef CM_HALL_CALIBRATION_SERVICE_H
#define CM_HALL_CALIBRATION_SERVICE_H

#include <Arduino.h>

#include "CM_HallTurnSource.h"

namespace CM
{

enum class HallCalibrationState : uint8_t
{
    Idle = 0U,
    WaitingLocalConfirm,
    ArmedWaitingPhysicalStart,
    Running,
    Completed,
    WaitingApplyConfirm,
    Aborted
};

enum class HallCalibrationDirection : uint8_t
{
    Rising = 0U,
    Falling
};

// Measurement-only result owned by Uno. Recommendation fields intentionally
// live on ESP32. measurementId binds a later proposal to this exact completed
// measurement and is deliberately transient across reboot.
struct HallCalibrationResult
{
    uint32_t measurementId;
    uint16_t baselineAdc;
    uint16_t minAdc;
    uint16_t maxAdc;
    uint16_t sampleCount;
    uint32_t durationMs;

    HallCalibrationResult();
};

class HallCalibrationService
{
public:
    explicit HallCalibrationService(HallTurnSource& hall);

    bool arm(uint32_t nowMs);
    void notePeerContact(uint32_t nowMs);
    bool confirmLocal(uint32_t nowMs);
    bool physicalStart(uint32_t nowMs);
    bool beginApplyConfirm(uint32_t measurementId, uint32_t nowMs);
    void completeApply();
    void update(uint32_t nowMs, bool safeEnvironment);
    void abort();
    void reset();

    HallCalibrationState state() const;
    bool active() const;
    bool motorPermit() const;
    bool baselineReady() const;
    bool takeResult(HallCalibrationResult& result);
    bool latestResult(HallCalibrationResult& result) const;

private:
    static constexpr uint16_t SampleIntervalMs = 10U;
    static constexpr uint16_t BaselineSampleIntervalMs = 20U;
    static constexpr uint8_t MinimumBaselineSamples = 8U;
    static constexpr uint32_t ArmedTimeoutMs = 60000UL;
    static constexpr uint32_t ApplyConfirmTimeoutMs = 30000UL;
    static constexpr uint32_t PeerTimeoutMs = 3000UL;
    static constexpr uint32_t RunDurationMs = 5000UL;
    static constexpr uint32_t AbsoluteRunTimeoutMs = 6500UL;

    void sampleBaseline(uint32_t nowMs);
    void sampleRunning(uint32_t nowMs);
    void finish(uint32_t nowMs);
    void clearMeasurements();
    bool populateResult(HallCalibrationResult& result) const;
    uint32_t measurementIdentity(const HallCalibrationResult& result) const;

    HallTurnSource& m_hall;
    HallCalibrationState m_state;
    bool m_resultPending;
    uint32_t m_armedAtMs;
    uint32_t m_lastPeerContactMs;
    uint32_t m_applyConfirmAtMs;
    uint32_t m_startedAtMs;
    uint32_t m_lastSampleMs;
    uint32_t m_baselineSum;
    uint16_t m_baselineSamples;
    uint16_t m_minAdc;
    uint16_t m_maxAdc;
    uint16_t m_runSamples;
    uint32_t m_resultDurationMs;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_SERVICE_H
