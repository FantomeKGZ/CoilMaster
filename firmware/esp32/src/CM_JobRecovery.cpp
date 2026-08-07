#include "CM_JobRecovery.h"

namespace CM
{
JobRecoveryInfo::JobRecoveryInfo()
    : disposition(JobRecoveryDisposition::None),
      state(),
      mayCreateNewJob(true),
      mayAutoQueue(false),
      mayAutoResume(false)
{
}

bool JobRecovery::evaluate(const JobStateStore& stateStore,
                           const JobSnapshotStore& snapshotStore,
                           JobRecoveryInfo& recovery)
{
    recovery = JobRecoveryInfo();

    bool found = false;
    JobRuntimeState latest;
    if (!stateStore.loadLatest(latest, found)) return false;
    if (!found) return true;

    // Runtime state is never trusted by itself. Recovery requires the immutable
    // source snapshot to exist, parse correctly and match the same identifiers.
    if (!snapshotStore.isReady() ||
        !snapshotStore.validateIdentity(latest.jobId, latest.sessionId))
    {
        recovery.mayCreateNewJob = false;
        return false;
    }

    recovery.state = latest;
    recovery.mayAutoQueue = false;
    recovery.mayAutoResume = false;

    if (requiresManualReview(latest))
    {
        recovery.disposition = JobRecoveryDisposition::ManualReviewRequired;
        recovery.mayCreateNewJob = false;
        return true;
    }

    recovery.disposition = JobRecoveryDisposition::RestoredForDisplay;
    recovery.mayCreateNewJob = isTerminalDelivery(latest.deliveryState) ||
        latest.executionState == JobExecutionState::ProgramCompleted ||
        latest.executionState == JobExecutionState::ClosedAfterReview;
    return true;
}

bool JobRecovery::requiresManualReview(const JobRuntimeState& state)
{
    if (state.executionState == JobExecutionState::ClosedAfterReview)
        return false;

    if (state.executionState == JobExecutionState::Running ||
        state.executionState == JobExecutionState::Fault)
    {
        return true;
    }

    // Delivery may have reached Arduino while ESP32 was restarting. Never resend
    // or replace such a job automatically because Arduino may already hold it.
    return state.deliveryState == JobDeliveryState::Delivering ||
           (state.deliveryState == JobDeliveryState::Accepted &&
            state.executionState == JobExecutionState::WaitingPhysicalStart);
}

bool JobRecovery::isTerminalDelivery(JobDeliveryState state)
{
    return state == JobDeliveryState::Rejected ||
           state == JobDeliveryState::TimedOut ||
           state == JobDeliveryState::Cancelled;
}
}
