#include "CM_HallCalibrationProtocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{
namespace HallCalibrationProtocol
{
namespace
{
bool parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool verifyAndStripCrc(char* frame)
{
    if (frame == nullptr) return false;
    char* lastSeparator = strrchr(frame, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t received = 0U;
    if (!parseHex16(lastSeparator + 1, received)) return false;

    const size_t payloadLength = static_cast<size_t>(lastSeparator - frame);
    const uint16_t calculated = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(frame), payloadLength);
    if (calculated != received) return false;

    *lastSeparator = '\0';
    return true;
}

bool appendCrc(char* output, size_t outputSize, int payloadLength)
{
    if (output == nullptr || outputSize == 0U || payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= outputSize)
    {
        return false;
    }

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output),
        static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf(
        output + payloadLength,
        outputSize - static_cast<size_t>(payloadLength),
        "|%04X\n",
        static_cast<unsigned int>(crc));
    return suffixLength > 0 &&
           static_cast<size_t>(payloadLength + suffixLength) < outputSize;
}
}

bool parseRequest(char* frame, HallCalibrationCommand& command)
{
    command = HallCalibrationCommand::None;
    if (!verifyAndStripCrc(frame)) return false;

    char* save = nullptr;
    char* version = strtok_r(frame, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* action = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || action == nullptr ||
        capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "CAL") != 0 ||
        strcmp(capability, "C") != 0)
    {
        return false;
    }

    if (strcmp(action, "ARM") == 0)
        command = HallCalibrationCommand::Arm;
    else if (strcmp(action, "ABORT") == 0)
        command = HallCalibrationCommand::Abort;
    else if (strcmp(action, "GET") == 0)
        command = HallCalibrationCommand::Get;
    else
        return false;

    return true;
}

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    const int length = snprintf(
        output,
        outputSize,
        "CMP1|CAL_STATE|%s|%u|%u|C",
        stateName(state),
        baselineReady ? 1U : 0U,
        motorPermit ? 1U : 0U);
    return appendCrc(output, outputSize, length);
}

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    // Keep the staged CMP1 CAL_RESULT shape stable while recommendation
    // ownership lives on ESP32. Uno sends measurement fields plus neutral
    // recommendation placeholders; ESP32 recomputes threshold/hysteresis/
    // direction from baseline/min/max.
    const int length = snprintf(
        output,
        outputSize,
        "CMP1|CAL_RESULT|INVALID|%u|%u|%u|0|0|RISING|%u|%lu|C",
        static_cast<unsigned int>(result.baselineAdc),
        static_cast<unsigned int>(result.minAdc),
        static_cast<unsigned int>(result.maxAdc),
        static_cast<unsigned int>(result.sampleCount),
        static_cast<unsigned long>(result.durationMs));
    return appendCrc(output, outputSize, length);
}

const char* stateName(HallCalibrationState state)
{
    switch (state)
    {
        case HallCalibrationState::ArmedWaitingPhysicalStart:
            return "ARMED_WAITING_START";
        case HallCalibrationState::Running:
            return "RUNNING";
        case HallCalibrationState::Completed:
            return "COMPLETED";
        case HallCalibrationState::Aborted:
            return "ABORTED";
        case HallCalibrationState::Idle:
        default:
            return "IDLE";
    }
}

const char* directionName(HallCalibrationDirection direction)
{
    return direction == HallCalibrationDirection::Falling ? "FALLING" : "RISING";
}

} // namespace HallCalibrationProtocol
} // namespace CM
