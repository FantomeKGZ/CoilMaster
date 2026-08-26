#include "CM_BackupActivityGuard.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_JobStateStore.h"
#include "CM_RunWireIssuePendingStore.h"

namespace CM
{
namespace
{
BackupActivityGuard::RuntimeProbe RuntimeActivityProbe = nullptr;

bool directoryReady(fs::FS& storage, const char* path)
{
    if (!storage.exists(path)) return false;
    File directory = storage.open(path, FILE_READ);
    if (!directory) return false;
    const bool ready = directory.isDirectory();
    directory.close();
    return ready;
}
}

void BackupActivityGuard::setRuntimeProbe(RuntimeProbe probe)
{
    RuntimeActivityProbe = probe;
}

BackupActivityCheck BackupActivityGuard::runtimeCheck()
{
    return RuntimeActivityProbe != nullptr
        ? RuntimeActivityProbe()
        : BackupActivityCheck::Unavailable;
}

BackupActivityCheck BackupActivityGuard::check(fs::FS& storage)
{
    const BackupActivityCheck runtime = runtimeCheck();
    if (runtime != BackupActivityCheck::Safe)
        return runtime;

    // A RUN_WIRE ISSUE pending owns a multi-store stock transaction. Never
    // snapshot or restore through the boundary while its authoritative intent
    // or atomic temp file is present; recovery must complete first.
    if (storage.exists(RunWireIssuePendingStore::Path) ||
        storage.exists(RunWireIssuePendingStore::TempPath))
    {
        return BackupActivityCheck::Busy;
    }

    if (!directoryReady(storage, "/data") ||
        !directoryReady(storage, "/data/winding-jobs") ||
        !directoryReady(storage, "/data/winding-jobs/state"))
    {
        return BackupActivityCheck::Unavailable;
    }

    JobStateStore states(storage);
    if (!states.begin() || !states.isReady())
        return BackupActivityCheck::Unavailable;

    JobRuntimeState latest;
    bool found = false;
    if (!states.loadLatest(latest, found))
        return BackupActivityCheck::Unavailable;
    if (!found)
        return BackupActivityCheck::Safe;

    // CREATED local preparation has not crossed the UART delivery boundary.
    // DELIVERING and later ambiguous/run states remain fail-closed Busy.
    const bool localPreparation = JobStateStore::isLocalPreparation(latest);
    const bool busy =
        latest.deliveryState == JobDeliveryState::Delivering ||
        latest.deliveryState == JobDeliveryState::TimedOut ||
        latest.executionState == JobExecutionState::WaitingPhysicalStart ||
        latest.executionState == JobExecutionState::Running ||
        latest.executionState == JobExecutionState::Fault;
    if (latest.executionState != JobExecutionState::ClosedAfterReview && busy)
        return BackupActivityCheck::Busy;

    if (!directoryReady(storage, "/data/winding-jobs/snapshots"))
        return BackupActivityCheck::Unavailable;

    JobSnapshotStore snapshots(storage);
    JobSnapshot snapshot;
    if (!snapshots.begin() || !snapshots.load(latest.sessionId, snapshot) ||
        snapshot.jobId != latest.jobId || snapshot.sessionId != latest.sessionId)
    {
        return BackupActivityCheck::Unavailable;
    }

    // A linked CREATED preparation may legitimately stop before exact spool
    // selection is committed. That residue is safe to supersede, but backup of
    // the incomplete preparation is not considered complete/authoritative.
    if (snapshot.linkage.linked)
    {
        if (!directoryReady(storage, "/data/winding-jobs/spool-selection"))
            return localPreparation ? BackupActivityCheck::Safe
                                    : BackupActivityCheck::Unavailable;

        JobSpoolSelection selection;
        bool selectionFound = false;
        if (!JobSpoolSelectionStore::loadReadOnly(storage,
                                                   latest.sessionId,
                                                   selection,
                                                   selectionFound))
        {
            return BackupActivityCheck::Unavailable;
        }
        if (!selectionFound)
            return localPreparation ? BackupActivityCheck::Safe
                                    : BackupActivityCheck::Unavailable;
        if (!selection.isValid() || selection.jobId != latest.jobId ||
            selection.sessionId != latest.sessionId ||
            selection.repairId != snapshot.linkage.repairId ||
            selection.motorId != snapshot.linkage.motorId)
        {
            return BackupActivityCheck::Unavailable;
        }
    }

    return BackupActivityCheck::Safe;
}
}
