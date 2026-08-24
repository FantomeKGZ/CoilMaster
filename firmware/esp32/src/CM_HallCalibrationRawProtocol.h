#ifndef CM_HALL_CALIBRATION_RAW_PROTOCOL_H
#define CM_HALL_CALIBRATION_RAW_PROTOCOL_H

#include <Arduino.h>

namespace CM
{

enum class HallCalibrationRawPhase : uint8_t
{
    Baseline = 0U,
    Run
};

struct HallCalibrationRawSample
{
    HallCalibrationRawPhase phase;
    uint16_t rawAdc;
    uint16_t sequence;
    uint32_t elapsedMs;
    bool valid;

    HallCalibrationRawSample();
};

namespace HallCalibrationRawProtocol
{
bool parseSample(char* line, HallCalibrationRawSample& sample);
}

} // namespace CM

#endif // CM_HALL_CALIBRATION_RAW_PROTOCOL_H
