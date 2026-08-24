#ifndef CM_HALL_CALIBRATION_RAW_BRIDGE_H
#define CM_HALL_CALIBRATION_RAW_BRIDGE_H

#include <Arduino.h>

#include "CM_HallCalibrationProtocol.h"

namespace CM
{

class UartEventTransport;

namespace HallCalibrationRawBridgeInternal
{
extern UartEventTransport* transport;
}

// Temporary migration bridge: keeps HallCalibrationService independent from
// main.cpp while raw calibration samples are moved to ESP32. It has no START
// or SSR authority and can only publish bounded CAL_SAMPLE frames.
class HallCalibrationRawBridgeRegistration
{
public:
    explicit HallCalibrationRawBridgeRegistration(UartEventTransport* owner)
    {
        HallCalibrationRawBridgeInternal::transport = owner;
    }
};

namespace HallCalibrationRawBridge
{
bool publish(HallCalibrationSamplePhase phase,
             uint16_t rawAdc,
             uint16_t sequence,
             uint32_t elapsedMs);
}

} // namespace CM

#endif // CM_HALL_CALIBRATION_RAW_BRIDGE_H
