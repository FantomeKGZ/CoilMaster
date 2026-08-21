#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::closeAfterRemoteCancel(uint32_t sessionId, uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    // A late duplicate CANCELLED/ALL_CLEAR must not turn already completed or
    // operator-closed run evidence into a storage fault. These states are
    // already terminal for the active-job lifecycle, so keep them unchanged.
    if (state.executionState == JobExecutionState::ProgramCompleted ||
        state.executionState == JobExecutionState::ClosedAfterReview)
    {
        return true;
    }

    // A positive Arduino CANCELLED/ALL_CLEAR acknowledgement proves only that
    // no remote job remains on the physical controller. It never proves a run
    // completed. Permit cancellation closure only while there is zero physical
    // run evidence. Running/inter-repeat states remain fail-closed.
    const bool waitingOnly =
        state.executionState == JobExecutionState::WaitingDelivery ||
        state.executionState == JobExecutionState::WaitingPhysicalStart;
    if (!waitingOnly || state.lastRunId != 0UL || state.completedRuns != 0U)
        return false;

    state.deliveryState = JobDeliveryState::Cancelled;
    state.executionState = JobExecutionState::ClosedAfterReview;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}
}
