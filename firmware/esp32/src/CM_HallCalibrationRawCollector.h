#ifndef CM_HALL_CALIBRATION_RAW_COLLECTOR_H
#define CM_HALL_CALIBRATION_RAW_COLLECTOR_H

#include <Arduino.h>

namespace CM
{

struct HallCalibrationRawSummary
{
    uint16_t baselineAdc;
    uint16_t minAdc;
    uint16_t maxAdc;
    uint16_t baselineSamples;
    uint16_t runSamples;
    uint32_t durationMs;
    bool valid;

    HallCalibrationRawSummary();
};

// ESP32-owned measurement accumulator for Hall calibration.
// Uno should eventually emit bounded raw ADC samples only; all baseline/min/max
// aggregation for the extended test belongs here, off the ATmega328P image.
class HallCalibrationRawCollector
{
public:
    static constexpr uint16_t MinimumBaselineSamples = 8U;
    static constexpr uint16_t MaxBaselineSamples = 4096U;

    HallCalibrationRawCollector();

    void reset();
    bool addBaselineSample(uint16_t rawAdc);
    void beginRun();
    bool addRunSample(uint16_t rawAdc);
    bool finish(uint32_t durationMs);
    HallCalibrationRawSummary summary() const;

private:
    uint32_t m_baselineSum;
    uint16_t m_baselineSamples;
    uint16_t m_minAdc;
    uint16_t m_maxAdc;
    uint16_t m_runSamples;
    uint32_t m_durationMs;
    bool m_running;
    bool m_finished;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_RAW_COLLECTOR_H
