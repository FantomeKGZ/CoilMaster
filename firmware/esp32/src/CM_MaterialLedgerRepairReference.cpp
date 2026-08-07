#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::repairExists(uint32_t repairId) const
{
    if (!ready() || repairId == 0UL || !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousId = 0UL;
    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidate = 0UL;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "repair_id", candidate) || candidate == 0UL ||
            candidate <= previousId)
        {
            file.close();
            return false;
        }
        previousId = candidate;
        if (candidate == repairId) found = true;
    }

    file.close();
    return found;
}
}
