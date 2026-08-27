#ifndef CM_HALL_CALIBRATION_SERVICE_H
#define CM_HALL_CALIBRATION_SERVICE_H

#include <Arduino.h>

#include "CM_HallTurnSource.h"

#ifndef CM_LCD_RU_EN
#define CM_LCD_RU_EN 0
#endif

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

// Uno owns only correlation identity for a completed calibration measurement.
// Measurement statistics are collected from CAL_SAMPLE on ESP32. The legacy
// CAL_RESULT wire fields remain for compatibility, but the formatter emits
// them as literal zeroes and ESP32 replaces them with its raw-stream summary.
struct HallCalibrationResult
{
    uint32_t measurementId;

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

#if CM_LCD_RU_EN
    static HallCalibrationState displayState();
    static uint32_t displayStartedAtMs();
    static constexpr uint32_t displayRunDurationMs() { return RunDurationMs; }
#endif

private:
    // Keep ADC sampling fast enough to catch narrow Hall peaks, but never try
    // to stream every sample over the 9600-baud SoftwareSerial link.
    static constexpr uint16_t SampleIntervalMs = 10U;
    static constexpr uint16_t RunPublishIntervalMs = 250U;
    static constexpr uint16_t BaselineSampleIntervalMs = 100U;
    static constexpr uint8_t MinimumBaselineSamples = 8U;
    static constexpr uint32_t ArmedTimeoutMs = 60000UL;
    static constexpr uint32_t ApplyConfirmTimeoutMs = 30000UL;
    static constexpr uint32_t PeerTimeoutMs = 10000UL;
    static constexpr uint32_t RunDurationMs = 15000UL;
    static constexpr uint32_t AbsoluteRunTimeoutMs = 17000UL;

    void sampleBaseline(uint32_t nowMs);
    void sampleRunning(uint32_t nowMs);
    void publishRunWindow(uint32_t nowMs);
    void finish(uint32_t nowMs);
    void clearMeasurements();
    bool populateResult(HallCalibrationResult& result) const;
    uint32_t measurementIdentity(uint32_t completedAtMs) const;

#if CM_LCD_RU_EN
    static HallCalibrationState s_displayState;
    static uint32_t s_displayStartedAtMs;
#endif

    HallTurnSource& m_hall;
    HallCalibrationState m_state;
    bool m_resultPending;
    uint32_t m_armedAtMs;
    uint32_t m_lastPeerContactMs;
    uint32_t m_applyConfirmAtMs;
    uint32_t m_startedAtMs;
    uint32_t m_lastSampleMs;
    uint32_t m_lastPublishMs;
    uint32_t m_measurementId;
    uint16_t m_runWindowMin;
    uint16_t m_runWindowMax;
    uint8_t m_baselineSamples;
    uint16_t m_runSamples;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_SERVICE_H
