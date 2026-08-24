#include "CM_HallCalibrationProtocol.h"

#include <stdio.h>
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
bool appendCrc(char* output, size_t outputSize, int payloadLength)
{
    if (output == nullptr || outputSize == 0U || payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= outputSize) return false;
    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output), static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf_P(
        output + payloadLength,
        outputSize - static_cast<size_t>(payloadLength),
        PSTR("|%04X\n"), static_cast<unsigned int>(crc));
    return suffixLength > 0 &&
           static_cast<size_t>(payloadLength + suffixLength) < outputSize;
}

bool appendP(char*& cursor, size_t& remaining, PGM_P text)
{
    const size_t length = strlen_P(text);
    if (length >= remaining) return false;
    memcpy_P(cursor, text, length);
    cursor += length;
    remaining -= length;
    *cursor = '\0';
    return true;
}

bool appendUnsigned(char*& cursor, size_t& remaining, uint32_t value)
{
    char digits[11];
    ultoa(static_cast<unsigned long>(value), digits, 10);
    const size_t length = strlen(digits);
    if (length >= remaining) return false;
    memcpy(cursor, digits, length);
    cursor += length;
    remaining -= length;
    *cursor = '\0';
    return true;
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
    HardwareControlRequest parsed;
    if (!HardwareControlProtocol::parseRequest(frame, parsed)) return false;

    switch (parsed.type)
    {
        case HardwareControlRequestType::ArmHallCalibration:
            command = HallCalibrationCommand::Arm;
            return true;
        case HardwareControlRequestType::AbortHallCalibration:
            command = HallCalibrationCommand::Abort;
            return true;
        case HardwareControlRequestType::GetHallCalibration:
            command = HallCalibrationCommand::Get;
            return true;
        default:
            return false;
    }
}

bool parseProposal(char* frame, HallCalibrationProposalRequest& proposal)
{
    proposal = HallCalibrationProposalRequest();
    HardwareControlRequest parsed;
    if (!HardwareControlProtocol::parseRequest(frame, parsed) ||
        parsed.type != HardwareControlRequestType::StageHallCalibrationProposal)
    {
        return false;
    }

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
    const int length = snprintf_P(output, outputSize,
        PSTR("CMP1|CAL_STATE|%S|%u|%u|C"), stateNameP(state),
        baselineReady ? 1U : 0U, motorPermit ? 1U : 0U);
    return appendCrc(output, outputSize, length);
}

bool formatSample(HallCalibrationSamplePhase phase,
                  uint16_t rawAdc,
                  uint16_t sequence,
                  uint32_t elapsedMs,
                  char* output,
                  size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || rawAdc > 1023U)
        return false;

    char* cursor = output;
    size_t remaining = outputSize;
    *cursor = '\0';

    if (!appendP(cursor, remaining, PSTR("CMP1|CAL_SAMPLE|")) ||
        !appendP(cursor, remaining,
                 phase == HallCalibrationSamplePhase::Run
                     ? PSTR("RUN|") : PSTR("BASELINE|")) ||
        !appendUnsigned(cursor, remaining, rawAdc) ||
        !appendP(cursor, remaining, PSTR("|")) ||
        !appendUnsigned(cursor, remaining, sequence) ||
        !appendP(cursor, remaining, PSTR("|")) ||
        !appendUnsigned(cursor, remaining, elapsedMs) ||
        !appendP(cursor, remaining, PSTR("|C")))
    {
        return false;
    }

    return appendCrc(output, outputSize,
                     static_cast<int>(cursor - output));
}

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || result.measurementId == 0UL)
        return false;
    const int length = snprintf_P(
        output, outputSize,
        PSTR("CMP1|CAL_RESULT|INVALID|0|0|0|0|0|RISING|0|0|%lu|C"),
        static_cast<unsigned long>(result.measurementId));
    return appendCrc(output, outputSize, length);
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
    const int length = snprintf_P(output, outputSize,
        PSTR("CMP1|CAL_APPLIED|%lu|%S|%u|%u|%u|%S|C"),
        static_cast<unsigned long>(measurementId), applyResultNameP(result),
        static_cast<unsigned int>(settings.hallThreshold),
        static_cast<unsigned int>(settings.hallHysteresis),
        static_cast<unsigned int>(settings.hallReleaseDebounceMs),
        settings.hallDirection == HallSignalDirection::Falling
            ? PSTR("FALLING") : PSTR("RISING"));
    return appendCrc(output, outputSize, length);
}

} // namespace HallCalibrationProtocol
} // namespace CM
