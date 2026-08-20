#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::closeAfterRemoteCancel(uint32_t sessionId,
                                           uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    if (state.deliveryState == JobDeliveryState::Cancelled &&
        state.executionState == JobExecutionState::WaitingDelivery &&
        state.lastRunId == 0UL && state.completedRuns == 0U)
    {
        return true;
    }

    const bool noPhysicalRunEvidence =
        state.lastRunId == 0UL && state.completedRuns == 0U &&
        (state.executionState == JobExecutionState::WaitingDelivery ||
         state.executionState == JobExecutionState::WaitingPhysicalStart);
    const bool cancellableDelivery =
        state.deliveryState == JobDeliveryState::Created ||
        state.deliveryState == JobDeliveryState::Delivering ||
        state.deliveryState == JobDeliveryState::Accepted ||
        state.deliveryState == JobDeliveryState::TimedOut;
    if (!noPhysicalRunEvidence || !cancellableDelivery) return false;

    // Arduino has positively confirmed that no remote job remains. Collapse the
    // active execution state back to WaitingDelivery so the persisted schema
    // remains internally consistent with terminal delivery=CANCELLED.
    state.deliveryState = JobDeliveryState::Cancelled;
    state.executionState = JobExecutionState::WaitingDelivery;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}

bool JobStateStore::dismissInactive(uint32_t sessionId,
                                    uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    if (state.executionState == JobExecutionState::ClosedAfterReview)
        return true;

    const bool terminalDelivery =
        state.deliveryState == JobDeliveryState::Rejected ||
        state.deliveryState == JobDeliveryState::Cancelled;
    const bool completedProgram =
        state.executionState == JobExecutionState::ProgramCompleted;
    if (!terminalDelivery && !completedProgram) return false;

    state.executionState = JobExecutionState::ClosedAfterReview;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}
}
