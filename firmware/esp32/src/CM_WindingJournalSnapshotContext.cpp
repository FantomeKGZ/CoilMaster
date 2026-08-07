#include "CM_WindingJournal.h"

#include "CM_JobSnapshotStore.h"

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
        snapshot.jobId == 0UL)
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
