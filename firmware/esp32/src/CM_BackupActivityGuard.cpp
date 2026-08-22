#include "CM_BackupActivityGuard.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_JobStateStore.h"

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
    // Persisted ESP32 job files cannot prove transient runtime inactivity: local
    // Arduino winding, an in-flight control event, or a runtime/storage fault may
    // exist only in RAM. Therefore Unavailable must never be promoted to Safe by
    // the persisted scan below. Runtime Safe is a prerequisite, not a hint.
    if (runtime != BackupActivityCheck::Safe)
        return runtime;

    // Runtime Safe alone is still insufficient. Revalidate the persisted latest
    // state/snapshot identity so damaged or ambiguous recovery state also fails
    // closed before backup/restore/FTP/settings operations proceed.
    if (!directoryReady(storage, "/data") ||
        !directoryReady(storage, "/data/winding-jobs") ||
        !directoryReady(storage, "/data/winding-jobs/state"))
    {
        return BackupActivityCheck::Unavailable;
    }

    // Directories already exist, so begin() is validation-only here and does not
    // create storage as a side effect of a backup safety check.
    JobStateStore states(storage);
    if (!states.begin() || !states.isReady())
        return BackupActivityCheck::Unavailable;

    JobRuntimeState latest;
    bool found = false;
    if (!states.loadLatest(latest, found))
        return BackupActivityCheck::Unavailable;
    if (!found)
        return BackupActivityCheck::Safe;

    // Fail closed on every persisted state where physical inactivity cannot be
    // proven. TIMED_OUT is ambiguous because Arduino may have accepted the JOB
    // while every acknowledgement was lost.
    const bool busy =
        latest.deliveryState == JobDeliveryState::Created ||
        latest.deliveryState == JobDeliveryState::Delivering ||
        latest.deliveryState == JobDeliveryState::TimedOut ||
        latest.executionState == JobExecutionState::WaitingPhysicalStart ||
        latest.executionState == JobExecutionState::Running ||
        latest.executionState == JobExecutionState::Fault;
    if (latest.executionState != JobExecutionState::ClosedAfterReview && busy)
        return BackupActivityCheck::Busy;

    // For an otherwise inactive state, storage identity must still be provable.
    // This matters especially after reboot: file export does not run the complete
    // deep audit on every request, so it must not trust RAM readiness alone.
    if (!directoryReady(storage, "/data/winding-jobs/snapshots"))
        return BackupActivityCheck::Unavailable;

    JobSnapshotStore snapshots(storage);
    JobSnapshot snapshot;
    if (!snapshots.begin() || !snapshots.load(latest.sessionId, snapshot) ||
        snapshot.jobId != latest.jobId || snapshot.sessionId != latest.sessionId)
    {
        return BackupActivityCheck::Unavailable;
    }

    if (snapshot.linkage.linked)
    {
        if (!directoryReady(storage, "/data/winding-jobs/spool-selection"))
            return BackupActivityCheck::Unavailable;

        JobSpoolSelection selection;
        bool selectionFound = false;
        if (!JobSpoolSelectionStore::loadReadOnly(storage,
                                                   latest.sessionId,
                                                   selection,
                                                   selectionFound) ||
            !selectionFound || !selection.isValid() ||
            selection.jobId != latest.jobId ||
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
