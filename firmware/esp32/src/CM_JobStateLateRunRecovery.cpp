#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::confirmStartedAfterDeliveryTimeout(uint32_t sessionId,
                                                        uint32_t runId,
                                                        uint32_t nowMs)
{
    if (sessionId == 0UL || runId == 0UL) return false;

    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    // Idempotent replay after the transition was already persisted.
    if (state.deliveryState == JobDeliveryState::Accepted &&
        state.executionState == JobExecutionState::Running &&
        state.lastRunId == runId && state.completedRuns == 0U)
    {
        return true;
    }

    // Only a delivery timeout with no previous physical-run evidence can be
    // reconciled from a later RUN_STARTED for this exact persisted session.
    if (state.deliveryState != JobDeliveryState::TimedOut ||
        state.executionState != JobExecutionState::WaitingDelivery ||
        state.lastRunId != 0UL || state.completedRuns != 0U)
    {
        return false;
    }

    state.deliveryState = JobDeliveryState::Accepted;
    state.executionState = JobExecutionState::Running;
    state.lastRunId = runId;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}
}
