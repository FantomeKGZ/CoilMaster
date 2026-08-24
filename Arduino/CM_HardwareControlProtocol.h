#ifndef CM_HARDWARE_CONTROL_PROTOCOL_H
#define CM_HARDWARE_CONTROL_PROTOCOL_H

#include <Arduino.h>

#include "CM_HallTelemetry.h"
#include "CM_HardwareSettings.h"

namespace CM
{

enum class HardwareControlRequestType : uint8_t
{
    None = 0U,
    GetHallSettings,
    SetHallSettings,
    ResetHallSettings,
    StartHallTelemetry,
    StopHallTelemetry,
    StageHallCalibrationProposal
};

struct HardwareControlRequest
{
    HardwareControlRequestType type;
    uint32_t measurementId;
    HardwareSettings settings;

    HardwareControlRequest();
};

enum class HardwareControlResult : uint8_t
{
    Applied = 0U,
    Busy,
    Invalid,
    PersistenceFailed,
    Unsupported
};

namespace HardwareControlProtocol
{
static constexpr size_t MaxFrameLength = 176U;

// Shared bounded CMP1 CRC owner for hardware-control and Hall CAL commands.
// On success the trailing |CRC is stripped in place before token parsing.
bool verifyAndStripCrc(char* frame);

bool parseRequest(char* frame, HardwareControlRequest& request);

bool formatSettingsState(const HardwareSettings& settings,
                         bool loadedFromEeprom,
                         char* output,
                         size_t outputSize);

bool formatSettingsResult(HardwareControlResult result,
                          char* output,
                          size_t outputSize);

bool formatHallTelemetry(const HallTelemetrySnapshot& snapshot,
                         char* output,
                         size_t outputSize);
}

} // namespace CM

#endif // CM_HARDWARE_CONTROL_PROTOCOL_H
