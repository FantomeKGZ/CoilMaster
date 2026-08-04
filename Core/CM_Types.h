#ifndef CM_TYPES_H
#define CM_TYPES_H

#include <stdint.h>

namespace CM
{
constexpr uint8_t MaxCoilsPerJob = 10U;
constexpr uint16_t MaxTurnsPerCoil = 9999U;

enum class WindingType : uint8_t
{
    Working = 0U,
    Starting = 1U
};

enum class JobSource : uint8_t
{
    LocalKeypad = 0U,
    Esp32Web = 1U
};

enum class MachineState : uint8_t
{
    EnterCoilCount = 0U,
    EnterTurns = 1U,
    Ready = 2U,
    Winding = 3U,
    Paused = 4U,
    ManualRun = 5U,
    CoilComplete = 6U,
    JobComplete = 7U,
    Fault = 8U
};

enum class JobStatus : uint8_t
{
    Empty = 0U,
    Editing = 1U,
    Ready = 2U,
    Running = 3U,
    Paused = 4U,
    Completed = 5U,
    Cancelled = 6U,
    Failed = 7U
};

enum class InputAction : uint8_t
{
    None = 0U,
    Digit,
    StartOrResume,
    ReturnHome,
    ToggleManual,
    Pause,
    Backspace,
    Confirm
};
}

#endif // CM_TYPES_H
