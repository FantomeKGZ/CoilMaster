#include "CM_JobStateStore.h"

namespace CM
{
bool JobStateStore::readPersisted(fs::FS& fileSystem,
                                  uint32_t sessionId,
                                  JobRuntimeState& state)
{
    state = JobRuntimeState();
    if (sessionId == 0UL) return false;

    String path = F("/data/winding-jobs/state/session-");
    path += sessionId;
    path += F(".json");

    File file = fileSystem.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    if (file.size() == 0U || file.size() >= 320U)
    {
        file.close();
        return false;
    }

    const String input = file.readString();
    file.close();

    JobStateStore parser(fileSystem);
    return parser.parse(input, state) && state.sessionId == sessionId;
}
}
