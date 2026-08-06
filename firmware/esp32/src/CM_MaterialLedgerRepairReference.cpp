#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::repairExists(uint32_t repairId) const
{
    if (!m_ready || repairId == 0UL || !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file)
    {
        return false;
    }

    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t candidate = 0UL;
        if (findUnsigned(line, "repair_id", candidate) && candidate == repairId)
        {
            found = true;
            break;
        }
    }

    file.close();
    return found;
}
}
