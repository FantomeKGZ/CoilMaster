#include "CM_RepairCosting.h"

namespace CM
{
bool RepairCosting::repairExists(uint32_t repairId) const
{
    if (!m_ready || repairId == 0UL || !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentRepairId = 0UL;
        if (findUnsigned(line, "repair_id", currentRepairId) &&
            currentRepairId == repairId)
        {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}
}
