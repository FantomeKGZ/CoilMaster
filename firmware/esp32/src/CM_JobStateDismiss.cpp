#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::dismissInactive(uint32_t sessionId, uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    if (state.executionState == JobExecutionState::ClosedAfterReview)
        return true;

    const bool terminalDelivery =
        state.deliveryState == JobDeliveryState::Rejected ||
        state.deliveryState == JobDeliveryState::TimedOut ||
        state.deliveryState == JobDeliveryState::Cancelled;
    const bool programCompleted =
        state.executionState == JobExecutionState::ProgramCompleted;

    // Never hide a job that Arduino may still hold or that may be running.
    if (!terminalDelivery && !programCompleted)
        return false;

    state.executionState = JobExecutionState::ClosedAfterReview;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}
}
