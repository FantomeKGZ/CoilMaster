#include "CM_JobSpoolSelectionStore.h"

namespace CM
{
bool JobSpoolSelectionStore::load(uint32_t sessionId,
                                  JobSpoolSelection& selection,
                                  bool& found) const
{
    selection = JobSpoolSelection();
    found = false;
    if (!isReady() || sessionId == 0UL) return false;

    const String path = selectionPath(sessionId);
    if (!m_storage.exists(path)) return true;

    File file = m_storage.open(path, FILE_READ);
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
}
