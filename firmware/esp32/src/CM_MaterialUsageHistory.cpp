#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::appendUsageHistoryJson(String& json,
                                            uint32_t repairId,
                                            uint32_t materialId,
                                            uint16_t limit,
                                            uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(UsagePath)) return true;
    if (limit == 0U) limit = 50U;

    File file = m_storage.open(UsagePath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentRepairId = 0UL;
        uint32_t currentMaterialId = 0UL;
        if (!findUnsigned(line, "repair_id", currentRepairId) ||
            !findUnsigned(line, "material_id", currentMaterialId))
        {
            continue;
        }
        if (repairId > 0UL && currentRepairId != repairId) continue;
        if (materialId > 0UL && currentMaterialId != materialId) continue;
        if (!first) json += ',';
        json += line;
        first = false;
        ++count;
    }

    file.close();
    return true;
}
}
