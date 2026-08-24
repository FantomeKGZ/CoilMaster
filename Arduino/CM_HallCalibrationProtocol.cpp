#include "CM_HallCalibrationProtocol.h"

#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "CM_HardwareControlProtocol.h"
#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{
namespace HallCalibrationProtocol
{
namespace
{
bool appendChar(char* output,
                size_t outputSize,
                size_t& length,
                char value)
{
    if (output == nullptr || length + 1U >= outputSize) return false;
    output[length++] = value;
    output[length] = '\0';
    return true;
}

bool appendText(char* output,
                size_t outputSize,
                size_t& length,
                const char* text)
{
    if (text == nullptr) return false;
    while (*text != '\0')
    {
        if (!appendChar(output, outputSize, length, *text++)) return false;
    }
    return true;
}

bool appendTextP(char* output,
                 size_t outputSize,
                 size_t& length,
                 PGM_P text)
{
    if (text == nullptr) return false;
    for (;;)
    {
        const char value = static_cast<char>(pgm_read_byte(text++));
        if (value == '\0') return true;
        if (!appendChar(output, outputSize, length, value)) return false;
    }
}

bool appendUnsigned(char* output,
                    size_t outputSize,
                    size_t& length,
                    uint32_t value)
{
    char digits[11];
    ultoa(static_cast<unsigned long>(value), digits, 10);
    return appendText(output, outputSize, length, digits);
}

bool appendFieldSeparator(char* output,
                          size_t outputSize,
                          size_t& length)
{
    return appendChar(output, outputSize, length, '|');
}

bool appendCrc(char* output, size_t outputSize, size_t& length)
{
    if (output == nullptr || outputSize == 0U || length == 0U) return false;
    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output), length);
    static const char HexDigits[] PROGMEM = "0123456789ABCDEF";

    if (!appendFieldSeparator(output, outputSize, length)) return false;
    for (int8_t shift = 12; shift >= 0; shift -= 4)
    {
        const uint8_t nibble = static_cast<uint8_t>((crc >> shift) & 0x0FU);
        const char digit = static_cast<char>(pgm_read_byte(HexDigits + nibble));
        if (!appendChar(output, outputSize, length, digit)) return false;
    }
    return appendChar(output, outputSize, length, '\n');
}

PGM_P stateNameP(HallCalibrationState state)
{
    switch (state)
    {
        case HallCalibrationState::WaitingLocalConfirm:
            return PSTR("WAITING_LOCAL_CONFIRM");
        case HallCalibrationState::ArmedWaitingPhysicalStart:
            return PSTR("ARMED_WAITING_START");
        case HallCalibrationState::Running:
            return PSTR("RUNNING");
        case HallCalibrationState::Completed:
            return PSTR("COMPLETED");
        case HallCalibrationState::WaitingApplyConfirm:
            return PSTR("WAITING_APPLY_CONFIRM");
        case HallCalibrationState::Aborted:
            return PSTR("ABORTED");
        case HallCalibrationState::Idle:
        default:
            return PSTR("IDLE");
    }
}

PGM_P applyResultNameP(HallCalibrationApplyResult result)
{
    switch (result)
    {
        case HallCalibrationApplyResult::Applied: return PSTR("APPLIED");
        case HallCalibrationApplyResult::Invalid: return PSTR("INVALID");
        case HallCalibrationApplyResult::IdentityMismatch:
            return PSTR("IDENTITY_MISMATCH");
        case HallCalibrationApplyResult::Busy: return PSTR("BUSY");
        case HallCalibrationApplyResult::PersistenceFailed:
            return PSTR("PERSISTENCE_FAILED");
        case HallCalibrationApplyResult::Cancelled: return PSTR("CANCELLED");
    }
    return PSTR("INVALID");
}
}

bool parseRequest(char* frame, HallCalibrationCommand& command)
{
    command = HallCalibrationCommand::None;
    if (!HardwareControlProtocol::verifyAndStripCrc(frame)) return false;
    char* save = nullptr;
    char* version = strtok_r(frame, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* action = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || action == nullptr ||
        capability == nullptr || extra != nullptr ||
        strcmp_P(version, PSTR("CMP1")) != 0 ||
        strcmp_P(category, PSTR("CAL")) != 0 ||
        strcmp_P(capability, PSTR("C")) != 0)
        return false;
    if (strcmp_P(action, PSTR("ARM")) == 0)
        command = HallCalibrationCommand::Arm;
    else if (strcmp_P(action, PSTR("ABORT")) == 0)
        command = HallCalibrationCommand::Abort;
    else if (strcmp_P(action, PSTR("GET")) == 0)
        command = HallCalibrationCommand::Get;
    else
        return false;
    return true;
}

bool parseProposal(char* frame, HallCalibrationProposalRequest& proposal)
{
    proposal = HallCalibrationProposalRequest();
    HardwareControlRequest parsed;
    if (!HardwareControlProtocol::parseRequest(frame, parsed) ||
        parsed.type != HardwareControlRequestType::StageHallCalibrationProposal)
        return false;
    proposal.measurementId = parsed.measurementId;
    proposal.settings = parsed.settings;
    return true;
}

bool formatState(HallCalibrationState state,
                 bool baselineReady,
                 bool motorPermit,
                 char* output,
                 size_t outputSize)
{
    if (output == nullptr || outputSize == 0U) return false;
    output[0] = '\0';
    size_t length = 0U;
    return appendTextP(output, outputSize, length, PSTR("CMP1|CAL_STATE|")) &&
           appendTextP(output, outputSize, length, stateNameP(state)) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendChar(output, outputSize, length, baselineReady ? '1' : '0') &&
           appendFieldSeparator(output, outputSize, length) &&
           appendChar(output, outputSize, length, motorPermit ? '1' : '0') &&
           appendTextP(output, outputSize, length, PSTR("|C")) &&
           appendCrc(output, outputSize, length);
}

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || result.measurementId == 0UL)
        return false;
    output[0] = '\0';
    size_t length = 0U;
    return appendTextP(output, outputSize, length,
                       PSTR("CMP1|CAL_RESULT|INVALID|")) &&
           appendUnsigned(output, outputSize, length, result.baselineAdc) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, result.minAdc) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, result.maxAdc) &&
           appendTextP(output, outputSize, length, PSTR("|0|0|RISING|")) &&
           appendUnsigned(output, outputSize, length, result.sampleCount) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, result.durationMs) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, result.measurementId) &&
           appendTextP(output, outputSize, length, PSTR("|C")) &&
           appendCrc(output, outputSize, length);
}

bool formatApplied(uint32_t measurementId,
                   HallCalibrationApplyResult result,
                   const HardwareSettings& settings,
                   char* output,
                   size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || measurementId == 0UL ||
        !settings.isValid())
        return false;
    output[0] = '\0';
    size_t length = 0U;
    return appendTextP(output, outputSize, length, PSTR("CMP1|CAL_APPLIED|")) &&
           appendUnsigned(output, outputSize, length, measurementId) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendTextP(output, outputSize, length, applyResultNameP(result)) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, settings.hallThreshold) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length, settings.hallHysteresis) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendUnsigned(output, outputSize, length,
                          settings.hallReleaseDebounceMs) &&
           appendFieldSeparator(output, outputSize, length) &&
           appendTextP(output, outputSize, length,
                       settings.hallDirection == HallSignalDirection::Falling
                           ? PSTR("FALLING") : PSTR("RISING")) &&
           appendTextP(output, outputSize, length, PSTR("|C")) &&
           appendCrc(output, outputSize, length);
}

} // namespace HallCalibrationProtocol
} // namespace CM
