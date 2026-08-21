#include "CM_WindingJournal.h"

#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"

namespace CM
{
JournalSaveResult WindingJournal::save(const RemoteWindingEvent& event)
{
    if (!m_ready)
        return JournalSaveResult::StorageUnavailable;

    JobSnapshotStore snapshots(m_fileSystem);
    if (!snapshots.begin())
        return JournalSaveResult::StorageUnavailable;

    JobSnapshot snapshot;
    if (event.sessionId == 0UL ||
        !snapshots.load(event.sessionId, snapshot) ||
        snapshot.sessionId != event.sessionId ||
        snapshot.jobId == 0UL || snapshot.repeatTarget == 0U)
    {
        return JournalSaveResult::InvalidTransition;
    }

    // Enforce the immutable repeat target before the NDJSON append. This keeps
    // a late/invalid RUN_STARTED from reopening a completed program and keeps a
    // RUN_COMPLETED beyond the planned repeat count out of the journal entirely.
    // Use the small per-session state file rather than another full journal scan.
    JobStateStore states(m_fileSystem);
    if (!states.begin())
        return JournalSaveResult::StorageUnavailable;

    JobRuntimeState runtime;
    if (!states.load(event.sessionId, runtime))
        return JournalSaveResult::StorageUnavailable;
    if (runtime.jobId != snapshot.jobId || runtime.sessionId != snapshot.sessionId)
        return JournalSaveResult::InvalidTransition;

    if (event.type == RemoteEventType::RunStarted &&
        (runtime.executionState == JobExecutionState::ProgramCompleted ||
         runtime.completedRuns >= snapshot.repeatTarget))
    {
        return JournalSaveResult::InvalidTransition;
    }
    if (event.type == RemoteEventType::RunCompleted &&
        event.completedRuns > snapshot.repeatTarget)
    {
        return JournalSaveResult::InvalidTransition;
    }

    WindingEventContext context;
    context.jobId = snapshot.jobId;
    context.linked = snapshot.linkage.linked;
    context.repairId = snapshot.linkage.repairId;
    context.motorId = snapshot.linkage.motorId;
    if (!context.isValid())
        return JournalSaveResult::InvalidTransition;

    return save(event, context);
}
}
