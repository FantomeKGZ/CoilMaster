#ifndef CM_HALL_CALIBRATION_PROTOCOL_H
#define CM_HALL_CALIBRATION_PROTOCOL_H

#include <Arduino.h>

#include "CM_HallCalibrationService.h"
#include "CM_HardwareSettings.h"

namespace CM
{

enum class HallCalibrationCommand : uint8_t
{
    None = 0U,
    Arm,
    Abort,
    Get
};

struct HallCalibrationProposalRequest
{
    uint32_t measurementId;
    HardwareSettings settings;

    HallCalibrationProposalRequest()
        : measurementId(0UL), settings()
    {
    }
};

enum class HallCalibrationApplyResult : uint8_t
{
    Applied = 0U,
    Invalid,
    IdentityMismatch,
    Busy,
    PersistenceFailed,
    Cancelled
};

namespace HallCalibrationProtocol
{
// Longest current response is CAL_RESULT at 85 bytes including CRC/newline.
// Keep bounded headroom without spending 176 bytes of Uno stack per send.
static constexpr size_t MaxFrameLength = 96U;

bool parseRequest(char* frame, HallCalibrationCommand& command);
bool parseProposal(char* frame, HallCalibrationProposalRequest& proposal);

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize);

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize);

bool formatApplied(uint32_t measurementId,
                   HallCalibrationApplyResult result,
                   const HardwareSettings& settings,
                   char* output,
                   size_t outputSize);
}

} // namespace CM

#endif // CM_HALL_CALIBRATION_PROTOCOL_H
