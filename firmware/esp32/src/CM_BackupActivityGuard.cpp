#include "CM_BackupActivityGuard.h"
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

BackupActivityCheck BackupActivityGuard::check(fs::FS& storage)
{
    if (RuntimeActivityProbe != nullptr)
    {
        const BackupActivityCheck runtime = RuntimeActivityProbe();
        if (runtime != BackupActivityCheck::Unavailable)
            return runtime;
    }

    if (!directoryReady(storage, "/data") ||
        !directoryReady(storage, "/data/winding-jobs") ||
        !directoryReady(storage, "/data/winding-jobs/state"))
    {
        return BackupActivityCheck::Unavailable;
    }

    // Fallback for callers that have not registered a runtime probe. Directories
    // already exist, so begin() validates persisted state without creating them.
    JobStateStore states(storage);
    if (!states.begin() || !states.isReady())
        return BackupActivityCheck::Unavailable;

    JobRuntimeState latest;
    bool found = false;
    if (!states.loadLatest(latest, found))
        return BackupActivityCheck::Unavailable;
    if (!found) return BackupActivityCheck::Safe;

    const bool busy =
        latest.deliveryState == JobDeliveryState::Created ||
        latest.deliveryState == JobDeliveryState::Delivering ||
        latest.executionState == JobExecutionState::WaitingPhysicalStart ||
        latest.executionState == JobExecutionState::Running;

    return busy ? BackupActivityCheck::Busy : BackupActivityCheck::Safe;
}
}
