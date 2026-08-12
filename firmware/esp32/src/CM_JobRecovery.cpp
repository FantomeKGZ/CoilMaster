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

    // A delivery timeout is ambiguous: Arduino may have accepted the JOB while
    // every ACK was lost. Treat it like an interrupted delivery and require the
    // operator to prove the machine is idle before another job can be created.
    if (state.deliveryState == JobDeliveryState::TimedOut)
        return true;

    // Delivery may have reached Arduino while ESP32 was restarting. Never resend
    // or replace such a job automatically because Arduino may already hold it.
    return state.deliveryState == JobDeliveryState::Delivering ||
           (state.deliveryState == JobDeliveryState::Accepted &&
            state.executionState == JobExecutionState::WaitingPhysicalStart);
}

bool JobRecovery::isTerminalDelivery(JobDeliveryState state)
{
    // REJECTED is an explicit Arduino decision and CANCELLED is persisted only
    // when the job is known not to be physically active. TIMED_OUT is not
    // terminal because loss of every ACK cannot prove whether Arduino accepted.
    return state == JobDeliveryState::Rejected ||
           state == JobDeliveryState::Cancelled;
}
}
