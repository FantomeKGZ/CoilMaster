#include "CM_WarehouseStore.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool WarehouseStore::repairExists(uint32_t repairId) const
{
    if (!m_ready || repairId == 0UL || !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", currentRepairId) ||
            currentRepairId == 0UL)
        {
            file.close();
            return false;
        }

        if (currentRepairId == repairId && ++matches > 1U)
        {
            file.close();
            return false;
        }
    }

    file.close();
    return matches == 1U;
}
}
