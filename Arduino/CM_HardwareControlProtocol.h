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
    ArmHallCalibration,
    AbortHallCalibration,
    GetHallCalibration,
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
// Longest active hardware-control TX is HALL_STATE with every numeric field at
// its type maximum, FALLING, RELEASE_DEBOUNCE, CRC and newline:
// CMP1|HALL_STATE|65535|65535|65535|65535|65535|65535|65535|FALLING|1|RELEASE_DEBOUNCE|65535|4294967295|C|FFFF\n
// = 113 wire bytes if arbitrary uint16 values are formatted. Runtime Hall
// telemetry values are semantically bounded to ADC/settings ranges, but the
// formatter itself accepts the struct directly, so keep the type-safe bound.
static constexpr size_t MaxFrameLength = 114U;
static_assert(MaxFrameLength == 114U,
              "Hardware-control frame bound must include wire bytes plus NUL");

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
