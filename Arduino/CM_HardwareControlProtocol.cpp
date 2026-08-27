#include "CM_HardwareControlProtocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "CM_CrcFrameText.h"
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
    if (text[0] == '0' && text[1] != '\0') return false;
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
    if (payloadLength <= 0) return false;
    return CrcFrameText::append(
        output, outputSize, static_cast<size_t>(payloadLength));
}

bool verifyAndStripCrcImpl(char* frame)
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
    if (strcmp_P(text, PSTR("RISING")) == 0)
        direction = HallSignalDirection::Rising;
    else if (strcmp_P(text, PSTR("FALLING")) == 0)
        direction = HallSignalDirection::Falling;
    else
        return false;
    return true;
}

PGM_P directionNameP(HallSignalDirection direction)
{
    return direction == HallSignalDirection::Falling
               ? PSTR("FALLING") : PSTR("RISING");
}

PGM_P rearmStateNameP(HallRearmState state)
{
    switch (state)
    {
        case HallRearmState::WaitingRelease: return PSTR("WAITING_RELEASE");
        case HallRearmState::ReleaseDebounce: return PSTR("RELEASE_DEBOUNCE");
        case HallRearmState::Armed:
        default: return PSTR("ARMED");
    }
}

PGM_P resultNameP(HardwareControlResult result)
{
    switch (result)
    {
        case HardwareControlResult::Applied: return PSTR("APPLIED");
        case HardwareControlResult::Busy: return PSTR("BUSY");
        case HardwareControlResult::Invalid: return PSTR("INVALID");
        case HardwareControlResult::PersistenceFailed:
            return PSTR("PERSISTENCE_FAILED");
        case HardwareControlResult::Unsupported:
        default: return PSTR("UNSUPPORTED");
    }
}
}

bool verifyAndStripCrc(char* frame)
{
    return verifyAndStripCrcImpl(frame);
}

bool parseRequest(char* frame, HardwareControlRequest& request)
{
    request = HardwareControlRequest();
    if (!verifyAndStripCrc(frame)) return false;

    char* save = nullptr;
    char* version = strtok_r(frame, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr ||
        strcmp_P(version, PSTR("CMP1")) != 0)
        return false;

    if (strcmp_P(category, PSTR("CAL")) == 0)
    {
        char* action = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (action == nullptr || capability == nullptr || extra != nullptr ||
            strcmp_P(capability, PSTR("C")) != 0)
            return false;
        if (strcmp_P(action, PSTR("ARM")) == 0)
            request.type = HardwareControlRequestType::ArmHallCalibration;
        else if (strcmp_P(action, PSTR("ABORT")) == 0)
            request.type = HardwareControlRequestType::AbortHallCalibration;
        else if (strcmp_P(action, PSTR("GET")) == 0)
            request.type = HardwareControlRequestType::GetHallCalibration;
        else
            return false;
        return true;
    }

    if (strcmp_P(category, PSTR("CFG_GET")) == 0 ||
        strcmp_P(category, PSTR("CFG_RESET")) == 0)
    {
        const bool get = strcmp_P(category, PSTR("CFG_GET")) == 0;
        char* target = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (target == nullptr || capability == nullptr || extra != nullptr ||
            strcmp_P(target, PSTR("HALL")) != 0 ||
            strcmp_P(capability, PSTR("C")) != 0)
            return false;
        request.type = get
            ? HardwareControlRequestType::GetHallSettings
            : HardwareControlRequestType::ResetHallSettings;
        return true;
    }

    if (strcmp_P(category, PSTR("CFG_SET")) == 0 ||
        strcmp_P(category, PSTR("CAL_PROPOSAL")) == 0)
    {
        const bool proposal = strcmp_P(category, PSTR("CAL_PROPOSAL")) == 0;
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
            strcmp_P(capability, PSTR("C")) != 0)
            return false;

        if (proposal)
        {
            if (!parseUint32(measurementOrTarget, request.measurementId) ||
                request.measurementId == 0UL)
                return false;
        }
        else if (strcmp_P(measurementOrTarget, PSTR("HALL")) != 0)
            return false;

        HardwareSettings parsed;
        if (!parseUint16(thresholdText, parsed.hallThreshold) ||
            !parseUint16(hysteresisText, parsed.hallHysteresis) ||
            !parseUint16(debounceText, parsed.hallReleaseDebounceMs) ||
            !parseDirection(directionText, parsed.hallDirection) ||
            !parsed.isValid())
            return false;

        request.type = proposal
            ? HardwareControlRequestType::StageHallCalibrationProposal
            : HardwareControlRequestType::SetHallSettings;
        request.settings = parsed;
        return true;
    }

    if (strcmp_P(category, PSTR("HALL_TELEM")) == 0)
    {
        char* action = strtok_r(nullptr, "|", &save);
        char* capability = strtok_r(nullptr, "|", &save);
        char* extra = strtok_r(nullptr, "|", &save);
        if (action == nullptr || capability == nullptr || extra != nullptr ||
            strcmp_P(capability, PSTR("C")) != 0)
            return false;
        if (strcmp_P(action, PSTR("START")) == 0)
            request.type = HardwareControlRequestType::StartHallTelemetry;
        else if (strcmp_P(action, PSTR("STOP")) == 0)
            request.type = HardwareControlRequestType::StopHallTelemetry;
        else
            return false;
        return true;
    }
    return false;
}

bool formatSettingsState(const HardwareSettings& settings,
                         bool loadedFromEeprom,
                         char* output,
                         size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || !settings.isValid()) return false;
    const int length = snprintf_P(output, outputSize,
        PSTR("CMP1|CFG_STATE|HALL|%u|%u|%u|%S|%S|C"),
        static_cast<unsigned int>(settings.hallThreshold),
        static_cast<unsigned int>(settings.hallHysteresis),
        static_cast<unsigned int>(settings.hallReleaseDebounceMs),
        directionNameP(settings.hallDirection),
        loadedFromEeprom ? PSTR("EEPROM") : PSTR("FACTORY"));
    return appendCrc(output, outputSize, length);
}

bool formatSettingsResult(HardwareControlResult result,
                          char* output,
                          size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    const bool success = result == HardwareControlResult::Applied;
    const int length = snprintf_P(output, outputSize,
        PSTR("CMP1|%S|HALL|%S|C"),
        success ? PSTR("CFG_ACK") : PSTR("CFG_NACK"), resultNameP(result));
    return appendCrc(output, outputSize, length);
}

bool formatHallTelemetry(const HallTelemetrySnapshot& snapshot,
                         char* output,
                         size_t outputSize)
{
    if (!snapshot.valid || output == nullptr || outputSize == 0U) return false;
    const int length = snprintf_P(output, outputSize,
        PSTR("CMP1|HALL_STATE|%u|%u|%u|%u|%u|%u|%u|%S|%u|%S|%u|%lu|C"),
        static_cast<unsigned int>(snapshot.rawAdc),
        static_cast<unsigned int>(snapshot.windowMin),
        static_cast<unsigned int>(snapshot.windowMax),
        static_cast<unsigned int>(snapshot.threshold),
        static_cast<unsigned int>(snapshot.hysteresis),
        static_cast<unsigned int>(snapshot.releaseBoundary),
        static_cast<unsigned int>(snapshot.releaseDebounceMs),
        snapshot.inverted ? PSTR("FALLING") : PSTR("RISING"),
        snapshot.magnetDetected ? 1U : 0U,
        rearmStateNameP(snapshot.rearmState),
        static_cast<unsigned int>(snapshot.sampleCount),
        static_cast<unsigned long>(snapshot.capturedAtMs));
    return appendCrc(output, outputSize, length);
}

} // namespace HardwareControlProtocol
} // namespace CM
