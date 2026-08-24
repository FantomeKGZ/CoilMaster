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

bool parseDecimal32(const char* text, uint32_t& value)
{
    value = 0UL;
    if (text == nullptr || *text == '\0') return false;
    for (const char* cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor < '0' || *cursor > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(*cursor - '0');
        if (value > (0xFFFFFFFFUL - digit) / 10UL) return false;
        value = value * 10UL + digit;
    }
    return true;
}

bool parseDecimal16(const char* text, uint16_t& value)
{
    uint32_t parsed = 0UL;
    if (!parseDecimal32(text, parsed) || parsed > 0xFFFFUL) return false;
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
        static_cast<size_t>(payloadLength) >= outputSize) return false;
    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output), static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf(
        output + payloadLength,
        outputSize - static_cast<size_t>(payloadLength),
        "|%04X\n", static_cast<unsigned int>(crc));
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
        strcmp(capability, "C") != 0) return false;
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

bool parseProposal(char* frame, HallCalibrationProposalRequest& proposal)
{
    proposal = HallCalibrationProposalRequest();
    if (!verifyAndStripCrc(frame)) return false;
    char* save = nullptr;
    char* version = strtok_r(frame, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* measurementText = strtok_r(nullptr, "|", &save);
    char* thresholdText = strtok_r(nullptr, "|", &save);
    char* hysteresisText = strtok_r(nullptr, "|", &save);
    char* debounceText = strtok_r(nullptr, "|", &save);
    char* directionText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || measurementText == nullptr ||
        thresholdText == nullptr || hysteresisText == nullptr ||
        debounceText == nullptr || directionText == nullptr || capability == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "CAL_PROPOSAL") != 0 || strcmp(capability, "C") != 0)
        return false;
    if (!parseDecimal32(measurementText, proposal.measurementId) ||
        proposal.measurementId == 0UL ||
        !parseDecimal16(thresholdText, proposal.settings.hallThreshold) ||
        !parseDecimal16(hysteresisText, proposal.settings.hallHysteresis) ||
        !parseDecimal16(debounceText, proposal.settings.hallReleaseDebounceMs))
        return false;
    if (strcmp(directionText, "RISING") == 0)
        proposal.settings.hallDirection = HallSignalDirection::Rising;
    else if (strcmp(directionText, "FALLING") == 0)
        proposal.settings.hallDirection = HallSignalDirection::Falling;
    else
        return false;
    return proposal.settings.isValid();
}

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    const int length = snprintf(output, outputSize,
        "CMP1|CAL_STATE|%s|%u|%u|C", stateName(state),
        baselineReady ? 1U : 0U, motorPermit ? 1U : 0U);
    return appendCrc(output, outputSize, length);
}

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || result.measurementId == 0UL)
        return false;
    // Keep the existing CAL_RESULT wire shape until ESP32 measurement-id parsing
    // and proposal sending land atomically. The identity already exists on Uno
    // but is not exposed by this staged frame yet.
    const int length = snprintf(output, outputSize,
        "CMP1|CAL_RESULT|INVALID|%u|%u|%u|0|0|RISING|%u|%lu|C",
        static_cast<unsigned int>(result.baselineAdc),
        static_cast<unsigned int>(result.minAdc),
        static_cast<unsigned int>(result.maxAdc),
        static_cast<unsigned int>(result.sampleCount),
        static_cast<unsigned long>(result.durationMs));
    return appendCrc(output, outputSize, length);
}

bool formatApplied(uint32_t measurementId,
                   HallCalibrationApplyResult result,
                   const HardwareSettings& settings,
                   char* output,
                   size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || measurementId == 0UL)
        return false;
    const int length = snprintf(output, outputSize,
        "CMP1|CAL_APPLIED|%lu|%s|%u|%u|%u|%s|C",
        static_cast<unsigned long>(measurementId), applyResultName(result),
        static_cast<unsigned int>(settings.hallThreshold),
        static_cast<unsigned int>(settings.hallHysteresis),
        static_cast<unsigned int>(settings.hallReleaseDebounceMs),
        settings.hallDirection == HallSignalDirection::Falling ? "FALLING" : "RISING");
    return appendCrc(output, outputSize, length);
}

const char* stateName(HallCalibrationState state)
{
    switch (state)
    {
        case HallCalibrationState::WaitingLocalConfirm: return "WAITING_LOCAL_CONFIRM";
        case HallCalibrationState::ArmedWaitingPhysicalStart: return "ARMED_WAITING_START";
        case HallCalibrationState::Running: return "RUNNING";
        case HallCalibrationState::Completed: return "COMPLETED";
        case HallCalibrationState::WaitingApplyConfirm: return "WAITING_APPLY_CONFIRM";
        case HallCalibrationState::Aborted: return "ABORTED";
        case HallCalibrationState::Idle:
        default: return "IDLE";
    }
}

const char* directionName(HallCalibrationDirection direction)
{
    return direction == HallCalibrationDirection::Falling ? "FALLING" : "RISING";
}

const char* applyResultName(HallCalibrationApplyResult result)
{
    switch (result)
    {
        case HallCalibrationApplyResult::Applied: return "APPLIED";
        case HallCalibrationApplyResult::Invalid: return "INVALID";
        case HallCalibrationApplyResult::IdentityMismatch: return "IDENTITY_MISMATCH";
        case HallCalibrationApplyResult::Busy: return "BUSY";
        case HallCalibrationApplyResult::PersistenceFailed: return "PERSISTENCE_FAILED";
        case HallCalibrationApplyResult::Cancelled: return "CANCELLED";
    }
    return "INVALID";
}

} // namespace HallCalibrationProtocol
} // namespace CM
