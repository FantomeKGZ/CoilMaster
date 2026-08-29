#include "CM_JobRecovery.h"

namespace CM
{
JobRecoveryInfo::JobRecoveryInfo()
    : disposition(JobRecoveryDisposition::None),
      state(),
      linkage(),
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
    // Retain its already-validated linkage so read-only closure checks do not
    // reopen the same immutable snapshot immediately after this proof.
    JobSnapshot snapshot;
    if (!snapshotStore.isReady() ||
        !snapshotStore.load(latest.sessionId, snapshot) ||
        snapshot.jobId != latest.jobId || snapshot.sessionId != latest.sessionId)
    {
        recovery.mayCreateNewJob = false;
        return false;
    }

    recovery.state = latest;
    recovery.linkage = snapshot.linkage;
    recovery.mayAutoQueue = false;
    recovery.mayAutoResume = false;

    // CREATED is persisted before exact spool selection and before the state is
    // promoted to DELIVERING. It is therefore evidence of interrupted local
    // preparation, not an ambiguous remote delivery. Keep the immutable files
    // for audit, do not auto-queue anything, and allow a fresh higher-ID job.
    if (JobStateStore::isLocalPreparation(latest))
    {
        recovery.disposition = JobRecoveryDisposition::None;
        recovery.mayCreateNewJob = true;
        return true;
    }

    // An operator-closed job remains on storage for audit/history, but it is no
    // longer an active machine job and must not reappear after reboot.
    if (latest.executionState == JobExecutionState::ClosedAfterReview)
    {
        recovery.disposition = JobRecoveryDisposition::None;
        recovery.mayCreateNewJob = true;
        return true;
    }

    if (requiresManualReview(latest))
    {
        recovery.disposition = JobRecoveryDisposition::ManualReviewRequired;
        recovery.mayCreateNewJob = false;
        return true;
    }

    recovery.disposition = JobRecoveryDisposition::RestoredForDisplay;
    recovery.mayCreateNewJob = isTerminalDelivery(latest.deliveryState) ||
        latest.executionState == JobExecutionState::ProgramCompleted;
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

    if (state.deliveryState == JobDeliveryState::TimedOut)
        return true;

    // DELIVERING is the first state that may cross the UART boundary. Reboot in
    // DELIVERING therefore remains ambiguous and requires physical review.
    return state.deliveryState == JobDeliveryState::Delivering ||
           (state.deliveryState == JobDeliveryState::Accepted &&
            state.executionState == JobExecutionState::WaitingPhysicalStart);
}

bool JobRecovery::isTerminalDelivery(JobDeliveryState state)
{
    return state == JobDeliveryState::Rejected ||
           state == JobDeliveryState::Cancelled;
}
}
