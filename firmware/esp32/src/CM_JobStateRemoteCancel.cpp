#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::closeAfterRemoteCancel(uint32_t sessionId, uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    if (state.deliveryState == JobDeliveryState::Cancelled &&
        state.executionState == JobExecutionState::ClosedAfterReview)
    {
        return true;
    }

    // Arduino cancellation is only meaningful before the first physical START.
    // Never rewrite evidence for a job that has already produced a run.
    if (state.deliveryState != JobDeliveryState::Accepted ||
        state.executionState != JobExecutionState::WaitingPhysicalStart ||
        state.lastRunId != 0UL || state.completedRuns != 0U)
    {
        return false;
    }

    state.deliveryState = JobDeliveryState::Cancelled;
    state.executionState = JobExecutionState::ClosedAfterReview;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}
}
