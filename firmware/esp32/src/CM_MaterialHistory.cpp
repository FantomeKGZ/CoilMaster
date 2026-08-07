#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::appendAdjustmentHistoryJson(String& json,
                                                 uint32_t materialId,
                                                 uint16_t limit,
                                                 uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(AdjustmentsPath)) return true;
    if (limit == 0U) limit = 20U;

    File file = m_storage.open(AdjustmentsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    uint32_t previousAdjustmentId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!line.startsWith("{") || !line.endsWith("}"))
        {
            file.close();
            return false;
        }

        uint32_t adjustmentId = 0UL;
        uint32_t currentMaterialId = 0UL;
        uint32_t added = 0UL;
        uint32_t stockBefore = 0UL;
        uint32_t stockAfter = 0UL;
        uint32_t priceBefore = 0UL;
        uint32_t priceAfter = 0UL;
        String type, status, currencyBefore, currencyAfter, timestamp;
        if (!findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
            adjustmentId <= previousAdjustmentId ||
            !findUnsigned(line, "material_id", currentMaterialId) || currentMaterialId == 0UL ||
            !findString(line, "type", type) || type != "ADJUSTMENT" ||
            !findString(line, "status", status) || status != "CONFIRMED" ||
            !findUnsigned(line, "added_quantity_milli", added) ||
            !findUnsigned(line, "stock_before_milli", stockBefore) ||
            !findUnsigned(line, "stock_after_milli", stockAfter) ||
            stockBefore > 0xFFFFFFFFUL - added || stockAfter != stockBefore + added ||
            !findUnsigned(line, "price_before_minor", priceBefore) || priceBefore == 0UL ||
            !findUnsigned(line, "price_after_minor", priceAfter) || priceAfter == 0UL ||
            !findString(line, "currency_before", currencyBefore) || currencyBefore.length() != 3U ||
            !findString(line, "currency_after", currencyAfter) || currencyAfter.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            return false;
        }
        previousAdjustmentId = adjustmentId;

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }

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
