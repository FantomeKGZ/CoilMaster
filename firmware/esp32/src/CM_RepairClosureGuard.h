#pragma once

#include <FS.h>

#include "CM_JobRecovery.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"

namespace CM
{
class RepairClosureGuard
{
public:
    static bool canClose(fs::FS& storage, uint32_t repairId, bool& allowed)
    {
        allowed = false;
        if (repairId == 0UL) return false;

        JobStateStore states(storage);
        JobSnapshotStore snapshots(storage);
        if (!states.begin() || !snapshots.begin() ||
            !states.isReady() || !snapshots.isReady())
        {
            return false;
        }

        JobRecoveryInfo recovery;
        if (!JobRecovery::evaluate(states, snapshots, recovery)) return false;
        if (recovery.disposition == JobRecoveryDisposition::None)
        {
            allowed = true;
            return true;
        }

        // evaluate() already loaded and validated the immutable source snapshot
        // for this exact job/session and retained its linkage. Reuse that proof
        // instead of reopening the same snapshot only to inspect repair linkage.
        if (!recovery.linkage.linked || recovery.linkage.repairId != repairId)
        {
            allowed = true;
            return true;
        }

        allowed = recovery.mayCreateNewJob &&
                  recovery.disposition != JobRecoveryDisposition::ManualReviewRequired;
        return true;
    }
};
}
