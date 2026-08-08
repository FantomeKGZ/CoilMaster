#include "CM_JobSpoolSelectionStore.h"

namespace CM
{
namespace
{
String readOnlySelectionPath(uint32_t sessionId)
{
    String path = F("/data/winding-jobs/spool-selection/session-");
    path += sessionId;
    path += F(".json");
    return path;
}
}

bool JobSpoolSelectionStore::loadReadOnly(fs::FS& storage,
                                          uint32_t sessionId,
                                          JobSpoolSelection& selection,
                                          bool& found)
{
    selection = JobSpoolSelection();
    found = false;
    if (sessionId == 0UL) return false;

    const String path = readOnlySelectionPath(sessionId);
    if (!storage.exists(path)) return true;

    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory() || file.size() == 0U || file.size() >= 512U)
    {
        if (file) file.close();
        return false;
    }
    const String input = file.readString();
    file.close();

    if (!parse(input, selection) || selection.sessionId != sessionId)
    {
        selection = JobSpoolSelection();
        return false;
    }
    found = true;
    return true;
}

bool JobSpoolSelectionStore::load(uint32_t sessionId,
                                  JobSpoolSelection& selection,
                                  bool& found) const
{
    selection = JobSpoolSelection();
    found = false;
    if (!isReady()) return false;
    return loadReadOnly(m_storage, sessionId, selection, found);
}
}
