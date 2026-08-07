#include "CM_JobDisplayRecovery.h"

namespace CM
{
RecoveredJobDisplay::RecoveredJobDisplay()
    : jobId(0UL),
      sessionId(0UL),
      type(RemoteJobType::Working),
      coilCount(0U),
      turns{},
      linkage()
{
}

bool JobDisplayRecovery::load(const JobSnapshotStore& snapshots,
                              uint32_t expectedJobId,
                              uint32_t expectedSessionId,
                              RecoveredJobDisplay& display)
{
    display = RecoveredJobDisplay();
    if (expectedJobId == 0UL || expectedSessionId == 0UL)
        return false;

    JobSnapshot snapshot;
    if (!snapshots.load(expectedSessionId, snapshot) ||
        snapshot.jobId != expectedJobId ||
        snapshot.sessionId != expectedSessionId ||
        snapshot.coilCount == 0U || snapshot.coilCount > 10U)
    {
        return false;
    }

    display.jobId = snapshot.jobId;
    display.sessionId = snapshot.sessionId;
    display.type = snapshot.type;
    display.coilCount = snapshot.coilCount;
    display.linkage = snapshot.linkage;
    for (uint8_t index = 0U; index < snapshot.coilCount; ++index)
        display.turns[index] = snapshot.turns[index];

    return true;
}
}
