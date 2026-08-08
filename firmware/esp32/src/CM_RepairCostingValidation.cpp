#include "CM_RepairCosting.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool RepairCosting::repairExists(uint32_t repairId, bool& found) const
{
    found = false;
    if (!ready() || repairId == 0UL || !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", currentRepairId) ||
            currentRepairId == 0UL || currentRepairId <= previousId)
        {
            file.close();
            found = false;
            return false;
        }
        previousId = currentRepairId;
        if (currentRepairId == repairId) found = true;
    }

    file.close();
    return true;
}

bool RepairCosting::repairExists(uint32_t repairId) const
{
    bool found = false;
    return repairExists(repairId, found) && found;
}
}
