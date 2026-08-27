#include "CM_HallCalibrationProtocol.h"

#include <stdio.h>
#include <avr/pgmspace.h>

#include "CM_CrcFrameText.h"
#include "CM_HardwareControlProtocol.h"

namespace CM
{
namespace HallCalibrationProtocol
{
namespace
{
bool appendCrc(char* output, size_t outputSize, int payloadLength)
{
    if (payloadLength <= 0) return false;
    return CrcFrameText::append(
        output, outputSize, static_cast<size_t>(payloadLength));
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
    const int length = snprintf_P(
        output, outputSize,
        PSTR("CMP1|CAL_SAMPLE|%S|%u|%u|%lu|C"),
        phase == HallCalibrationSamplePhase::Run ? PSTR("RUN") : PSTR("BASELINE"),
        static_cast<unsigned int>(rawAdc),
        static_cast<unsigned int>(sequence),
        static_cast<unsigned long>(elapsedMs));
    return appendCrc(output, outputSize, length);
}

bool formatResult(const HallCalibrationResult& result,
                  char* output,
                  size_t outputSize)
{
    return formatDone(result, output, outputSize);
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
