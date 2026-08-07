#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::repairExists(uint32_t repairId, bool& found) const
{
    found = false;
    if (!ready() || repairId == 0UL || !m_storage.exists(RepairsPath))
        return false;

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "repair_id", currentId) || currentId == 0UL ||
            currentId <= previousId)
        {
            file.close();
            found = false;
            return false;
        }
        previousId = currentId;
        if (currentId == repairId) found = true;
    }

    file.close();
    return true;
}
}
