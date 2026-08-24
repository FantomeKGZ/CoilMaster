#include "CM_HallCalibrationRawBridge.h"

#include "CM_UartEventTransport.h"

namespace CM
{
namespace
{
UartEventTransport* g_rawCalibrationTransport = nullptr;
}

HallCalibrationRawBridgeRegistration::HallCalibrationRawBridgeRegistration(
    UartEventTransport* owner)
{
    g_rawCalibrationTransport = owner;
}

namespace HallCalibrationRawBridge
{
bool publish(HallCalibrationSamplePhase phase,
             uint16_t rawAdc,
             uint16_t sequence,
             uint32_t elapsedMs)
{
    return g_rawCalibrationTransport != nullptr &&
           g_rawCalibrationTransport->sendHallCalibrationSample(
               phase, rawAdc, sequence, elapsedMs);
}
}

} // namespace CM
