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
    if (!ready()) return false;
    if (!m_storage.exists(UsagePath)) return true;
    if (limit == 0U) limit = 50U;

    File file = m_storage.open(UsagePath, FILE_READ);
    if (!file) return false;

    bool first = true;
    uint32_t previousUsageId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!line.startsWith("{") || !line.endsWith("}"))
        {
            file.close();
            return false;
        }

        uint32_t usageId = 0UL;
        uint32_t currentRepairId = 0UL;
        uint32_t currentMaterialId = 0UL;
        uint32_t quantity = 0UL;
        uint32_t unitPrice = 0UL;
        String currency, timestamp;
        if (!findUnsigned(line, "usage_id", usageId) || usageId == 0UL ||
            usageId <= previousUsageId ||
            !findUnsigned(line, "repair_id", currentRepairId) || currentRepairId == 0UL ||
            !findUnsigned(line, "material_id", currentMaterialId) || currentMaterialId == 0UL ||
            !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", unitPrice) || unitPrice == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            return false;
        }
        previousUsageId = usageId;

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }

        if (repairId > 0UL && currentRepairId != repairId) continue;
        if (materialId > 0UL && currentMaterialId != materialId) continue;
        if (count >= limit) continue;
        if (!first) json += ',';
        json += line;
        first = false;
        ++count;
    }

    file.close();
    return true;
}
}
