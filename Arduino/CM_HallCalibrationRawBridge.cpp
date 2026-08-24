#include "CM_HallCalibrationRawBridge.h"

#include "CM_UartEventTransport.h"

namespace CM
{
namespace HallCalibrationRawBridgeInternal
{
UartEventTransport* transport = nullptr;
}

namespace HallCalibrationRawBridge
{
bool publish(HallCalibrationSamplePhase phase,
             uint16_t rawAdc,
             uint16_t sequence,
             uint32_t elapsedMs)
{
    return HallCalibrationRawBridgeInternal::transport != nullptr &&
           HallCalibrationRawBridgeInternal::transport->sendHallCalibrationSample(
               phase, rawAdc, sequence, elapsedMs);
}
}

} // namespace CM
