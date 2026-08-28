#include "CM_JobSnapshotStore.h"

namespace CM
{
bool JobSnapshotStore::readPersisted(fs::FS& fileSystem,
                                     uint32_t sessionId,
                                     JobSnapshot& snapshot)
{
    snapshot = JobSnapshot();
    if (sessionId == 0UL) return false;

    String path = F("/data/winding-jobs/snapshots/session-");
    path += sessionId;
    path += F(".json");

    JobSnapshotStore parser(fileSystem);
    return parser.readAndParse(path.c_str(), snapshot) &&
           snapshot.sessionId == sessionId;
}
}
