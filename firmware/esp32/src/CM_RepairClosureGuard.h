#pragma once

#include <FS.h>

#include "CM_JobDisplayRecovery.h"
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

        RecoveredJobDisplay display;
        if (!JobDisplayRecovery::load(snapshots,
                                      recovery.state.jobId,
                                      recovery.state.sessionId,
                                      display))
        {
            return false;
        }

        if (!display.linkage.linked || display.linkage.repairId != repairId)
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
