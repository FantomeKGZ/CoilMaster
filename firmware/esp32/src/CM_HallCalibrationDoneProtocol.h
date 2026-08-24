#ifndef CM_HALL_CALIBRATION_DONE_PROTOCOL_H
#define CM_HALL_CALIBRATION_DONE_PROTOCOL_H

#include <Arduino.h>

namespace CM
{

struct HallCalibrationDone
{
    uint32_t measurementId;
    bool valid;

    HallCalibrationDone();
};

namespace HallCalibrationDoneProtocol
{
// Compact completion/correlation frame for ESP32-owned raw aggregation.
// Carries no actuator command and no measurement statistics.
bool parseDone(char* line, HallCalibrationDone& done);
}

} // namespace CM

#endif // CM_HALL_CALIBRATION_DONE_PROTOCOL_H
