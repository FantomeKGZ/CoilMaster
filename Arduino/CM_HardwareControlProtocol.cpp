#include "CM_HardwareControlProtocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{

HardwareControlRequest::HardwareControlRequest()
    : type(HardwareControlRequestType::None), measurementId(0UL), settings()
{
}

namespace HardwareControlProtocol
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

bool parseUint32(const char* text, uint32_t& value)
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

bool parseUint16(const char* text, uint16_t& value)
{
    uint32_t parsed = 0UL;
    if (!parseUint32(text, parsed) || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool appendCrc(char* output, size_t outputSize, int payloadLength)
{
    if (output == nullptr || outputSize == 0U || payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= outputSize) return false;
    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output), static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf(output + payloadLength,
                                      outputSize - static_cast<size_t>(payloadLength),
                                      "|%04X\n", static_cast<unsigned int>(crc));
    return suffixLength > 0 &&
           static_cast<size_t>(payloadLength + suffixLength) < outputSize;
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

bool parseDirection(const char* text, HallSignalDirection& direction)
{
    if (text == nullptr) return false;
    if (strcmp(text, "RISING") == 0)
        direction = HallSignalDirection::Rising;
    else if (strcmp(text, "FALLING") == 0)
        direction = HallSignalDirection::Falling;
    else
        return false;
    return true;
}
}

bool parseRequest(char* frame, HardwareControlRequest& request)
{
    request = HardwareControlRequest();
    if (!verifyAndStripCrc(frame)) return false;

    char* save = nullptr;
    char* version = strtok_r(frame, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || strcmp(version, "CMP1") != 0)
        return false;

    if (strcmp(category, "CFG_GET") == 0 || strcmp(category, "CFG_RESET") == 0)
    {
        char* target = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (target == nullptr || capability == nullptr || extra != nullptr ||
            strcmp(target, "HALL") != 0 || strcmp(capability, "C") != 0) return false;
        request.type = strcmp(category, "CFG_GET") == 0
            ? HardwareControlRequestType::GetHallSettings
            : HardwareControlRequestType::ResetHallSettings;
        return true;
    }

    if (strcmp(category, "CFG_SET") == 0 || strcmp(category, "CAL_PROPOSAL") == 0)
    {
        const bool proposal = strcmp(category, "CAL_PROPOSAL") == 0;
        char* measurementOrTarget = strtok_r(nullptr, "|", &save);
        char* thresholdText = strtok_r(nullptr, "|", &save);
        char* hysteresisText = strtok_r(nullptr, "|", &save);
        char* debounceText = strtok_r(nullptr, "|", &save);
        char* directionText = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (measurementOrTarget == nullptr || thresholdText == nullptr ||
            hysteresisText == nullptr || debounceText == nullptr ||
            directionText == nullptr || capability == nullptr || extra != nullptr ||
            strcmp(capability, "C") != 0) return false;

        if (proposal)
        {
            if (!parseUint32(measurementOrTarget, request.measurementId) ||
                request.measurementId == 0UL) return false;
        }
        else if (strcmp(measurementOrTarget, "HALL") != 0)
            return false;

        HardwareSettings parsed;
        if (!parseUint16(thresholdText, parsed.hallThreshold) ||
            !parseUint16(hysteresisText, parsed.hallHysteresis) ||
            !parseUint16(debounceText, parsed.hallReleaseDebounceMs) ||
            !parseDirection(directionText, parsed.hallDirection) || !parsed.isValid())
            return false;

        request.type = proposal
            ? HardwareControlRequestType::StageHallCalibrationProposal
            : HardwareControlRequestType::SetHallSettings;
        request.settings = parsed;
        return true;
    }

    if (strcmp(category, "HALL_TELEM") == 0)
    {
        char* action = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (action == nullptr || capability == nullptr || extra != nullptr ||
            strcmp(capability, "C") != 0) return false;
        if (strcmp(action, "START") == 0)
            request.type = HardwareControlRequestType::StartHallTelemetry;
        else if (strcmp(action, "STOP") == 0)
            request.type = HardwareControlRequestType::StopHallTelemetry;
        else
            return false;
        return true;
    }
    return false;
}

bool formatSettingsState(const HardwareSettings& settings, bool loadedFromEeprom,
                         char* output, size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || !settings.isValid()) return false;
    const int length = snprintf(output, outputSize,
        "CMP1|CFG_STATE|HALL|%u|%u|%u|%s|%s|C",
        static_cast<unsigned int>(settings.hallThreshold),
        static_cast<unsigned int>(settings.hallHysteresis),
        static_cast<unsigned int>(settings.hallReleaseDebounceMs),
        directionName(settings.hallDirection), loadedFromEeprom ? "EEPROM" : "FACTORY");
    return appendCrc(output, outputSize, length);
}

bool formatSettingsResult(HardwareControlResult result, char* output, size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    const bool success = result == HardwareControlResult::Applied;
    const int length = snprintf(output, outputSize, "CMP1|%s|HALL|%s|C",
        success ? "CFG_ACK" : "CFG_NACK", resultName(result));
    return appendCrc(output, outputSize, length);
}

bool formatHallTelemetry(const HallTelemetrySnapshot& snapshot,
                         char* output, size_t outputSize)
{
    if (!snapshot.valid || output == nullptr || outputSize == 0U) return false;
    const int length = snprintf(output, outputSize,
        "CMP1|HALL_STATE|%u|%u|%u|%u|%u|%u|%u|%s|%u|%s|%u|%lu|C",
        static_cast<unsigned int>(snapshot.rawAdc),
        static_cast<unsigned int>(snapshot.windowMin),
        static_cast<unsigned int>(snapshot.windowMax),
        static_cast<unsigned int>(snapshot.threshold),
        static_cast<unsigned int>(snapshot.hysteresis),
        static_cast<unsigned int>(snapshot.releaseBoundary),
        static_cast<unsigned int>(snapshot.releaseDebounceMs),
        snapshot.inverted ? "FALLING" : "RISING",
        snapshot.magnetDetected ? 1U : 0U, rearmStateName(snapshot.rearmState),
        static_cast<unsigned int>(snapshot.sampleCount),
        static_cast<unsigned long>(snapshot.capturedAtMs));
    return appendCrc(output, outputSize, length);
}

const char* directionName(HallSignalDirection direction)
{
    return direction == HallSignalDirection::Falling ? "FALLING" : "RISING";
}

const char* rearmStateName(HallRearmState state)
{
    switch (state)
    {
        case HallRearmState::WaitingRelease: return "WAITING_RELEASE";
        case HallRearmState::ReleaseDebounce: return "RELEASE_DEBOUNCE";
        case HallRearmState::Armed:
        default: return "ARMED";
    }
}

const char* resultName(HardwareControlResult result)
{
    switch (result)
    {
        case HardwareControlResult::Applied: return "APPLIED";
        case HardwareControlResult::Busy: return "BUSY";
        case HardwareControlResult::Invalid: return "INVALID";
        case HardwareControlResult::PersistenceFailed: return "PERSISTENCE_FAILED";
        case HardwareControlResult::Unsupported:
        default: return "UNSUPPORTED";
    }
}

} // namespace HardwareControlProtocol
} // namespace CM
