#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool MaterialLedger::appendAdjustmentHistoryPageJson(
    String& json,
    uint32_t materialId,
    uint32_t cursor,
    uint8_t limit,
    uint16_t& count,
    uint32_t& nextCursor,
    bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize) return false;
    if (!m_storage.exists(AdjustmentsPath)) return cursor == 0UL;

    File file = m_storage.open(AdjustmentsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL || cursor > rawSize)
    {
        file.close();
        return false;
    }
    const uint32_t fileSize = static_cast<uint32_t>(rawSize);
    if (fileSize > 0UL &&
        (!file.seek(fileSize - 1UL) || file.read() != '\n'))
    {
        file.close();
        return false;
    }
    if (cursor > 0UL &&
        (!file.seek(cursor - 1UL) || file.read() != '\n'))
    {
        file.close();
        return false;
    }
    if (!file.seek(cursor))
    {
        file.close();
        return false;
    }

    bool first = true;
    uint32_t previousAdjustmentId = 0UL;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
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
        if (!findUnsigned(line, "adjustment_id", adjustmentId) ||
            adjustmentId == 0UL || adjustmentId <= previousAdjustmentId ||
            !findUnsigned(line, "material_id", currentMaterialId) ||
            currentMaterialId == 0UL ||
            !findString(line, "type", type) || type != "ADJUSTMENT" ||
            !findString(line, "status", status) || status != "CONFIRMED" ||
            !findUnsigned(line, "added_quantity_milli", added) ||
            !findUnsigned(line, "stock_before_milli", stockBefore) ||
            !findUnsigned(line, "stock_after_milli", stockAfter) ||
            stockBefore > 0xFFFFFFFFUL - added ||
            stockAfter != stockBefore + added ||
            !findUnsigned(line, "price_before_minor", priceBefore) ||
            priceBefore == 0UL ||
            !findUnsigned(line, "price_after_minor", priceAfter) ||
            priceAfter == 0UL ||
            !findString(line, "currency_before", currencyBefore) ||
            currencyBefore.length() != 3U ||
            !findString(line, "currency_after", currencyAfter) ||
            currencyAfter.length() != 3U ||
            !findString(line, "timestamp", timestamp) ||
            timestamp.length() < 10U)
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
        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }

    const uint32_t pageEnd = static_cast<uint32_t>(file.position());
    hasMore = pageEnd < fileSize;
    nextCursor = hasMore ? pageEnd : 0UL;
    file.close();
    return true;
}
}
