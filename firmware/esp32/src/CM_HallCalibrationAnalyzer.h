#ifndef CM_HALL_CALIBRATION_ANALYZER_H
#define CM_HALL_CALIBRATION_ANALYZER_H

#include <Arduino.h>

#include "CM_HardwareControlClient.h"

namespace CM
{

struct HallCalibrationProposal
{
    bool valid;
    uint16_t baselineAdc;
    uint16_t minAdc;
    uint16_t maxAdc;
    uint16_t recommendedThreshold;
    uint16_t recommendedHysteresis;
    HallSignalDirectionRemote direction;
    uint16_t sampleCount;
    uint32_t durationMs;

    HallCalibrationProposal();
};

class HallCalibrationAnalyzer
{
public:
    HallCalibrationAnalyzer();

    void reset();
    bool addBaselineSample(uint16_t rawAdc);
    bool baselineReady() const;
    void beginRun();
    void addRunSample(uint16_t rawAdc);
    HallCalibrationProposal finish(uint32_t durationMs) const;

private:
    static constexpr uint8_t MinimumBaselineSamples = 8U;
    static constexpr uint16_t MaximumBaselineSamples = 64U;
    static constexpr uint16_t MinimumSignalSpan = 60U;

    uint32_t m_baselineSum;
    uint16_t m_baselineSamples;
    uint16_t m_minAdc;
    uint16_t m_maxAdc;
    uint16_t m_runSamples;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_ANALYZER_H
