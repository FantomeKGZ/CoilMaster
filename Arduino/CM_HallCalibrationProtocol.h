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

enum class HallCalibrationSamplePhase : uint8_t
{
    Baseline = 0U,
    Run
};

namespace HallCalibrationProtocol
{
// Longest current response is CAL_RESULT at 85 bytes including CRC/newline.
// Keep bounded headroom without spending 176 bytes of Uno stack per send.
static constexpr size_t MaxFrameLength = 96U;

// Thin compatibility adapters only. Parsing/CRC ownership lives in
// HardwareControlProtocol so CAL and hardware-control frames do not carry
// duplicate parser implementations in the Uno image.
bool parseRequest(char* frame, HallCalibrationCommand& command);
bool parseProposal(char* frame, HallCalibrationProposalRequest& proposal);

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize);

// Compact calibration-only raw sample. It carries no actuator semantics and
// is intended for ESP32-owned extended analysis only.
bool formatSample(HallCalibrationSamplePhase phase,
                  uint16_t rawAdc,
                  uint16_t sequence,
                  uint32_t elapsedMs,
                  char* output,
                  size_t outputSize);

// Compact completion/correlation frame for ESP32-owned raw aggregation.
// Legacy formatResult remains active until ESP32 dual-frame receive is wired.
bool formatDone(const HallCalibrationResult& result,
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
