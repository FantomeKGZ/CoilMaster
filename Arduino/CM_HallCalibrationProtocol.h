#ifndef CM_HALL_CALIBRATION_PROTOCOL_H
#define CM_HALL_CALIBRATION_PROTOCOL_H

#include <Arduino.h>

#include "CM_HallCalibrationService.h"

namespace CM
{

enum class HallCalibrationCommand : uint8_t
{
    None = 0U,
    Arm,
    Abort,
    Get
};

namespace HallCalibrationProtocol
{
static constexpr size_t MaxFrameLength = 176U;

bool parseRequest(char* frame, HallCalibrationCommand& command);

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize);

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize);

const char* stateName(HallCalibrationState state);
const char* directionName(HallCalibrationDirection direction);
}

} // namespace CM

#endif // CM_HALL_CALIBRATION_PROTOCOL_H
