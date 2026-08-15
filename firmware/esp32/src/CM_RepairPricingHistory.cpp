#include "CM_RepairCosting.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool RepairCosting::appendPricingRevisionsPageJson(
    String& json,
    uint32_t repairId,
    uint32_t cursor,
    uint8_t limit,
    uint16_t& pageCount,
    uint16_t& totalCount,
    uint32_t& nextCursor,
    bool& hasMore,
    PricingRevisionSnapshot& latest) const
{
    pageCount = 0U;
    totalCount = 0U;
    nextCursor = 0UL;
    hasMore = false;
    latest = PricingRevisionSnapshot();
    if (!ready() || repairId == 0UL || limit == 0U ||
        limit > MaxPricingHistoryPageSize) return false;
    if (!m_storage.exists(PricingPath)) return cursor == 0UL;

    File file = m_storage.open(PricingPath, FILE_READ);
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
    if (!file.seek(0UL))
    {
        file.close();
        return false;
    }

    bool first = true;
    while (file.available())
    {
        const uint32_t lineStart = static_cast<uint32_t>(file.position());
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t lineRepairId = 0UL;
        uint64_t labour = 0ULL;
        uint64_t clientPrice = 0ULL;
        String currency;
        String timestamp;

        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", lineRepairId) || lineRepairId == 0UL ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", clientPrice) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            pageCount = 0U;
            totalCount = 0U;
            latest = PricingRevisionSnapshot();
            return false;
        }

        if (lineRepairId != repairId) continue;
        if (totalCount == 0xFFFFU)
        {
            file.close();
            pageCount = 0U;
            totalCount = 0U;
            latest = PricingRevisionSnapshot();
            return false;
        }

        ++totalCount;
        latest.labourCostMinor = labour;
        latest.clientPriceMinor = clientPrice;
        latest.currency = currency;
        latest.timestamp = timestamp;

        if (lineStart < cursor) continue;
        if (pageCount >= limit)
        {
            hasMore = true;
            continue;
        }

        if (!first) json += ',';
        first = false;
        json += F("{\"revision\":"); json += totalCount;
        json += F(",\"labour_cost_minor\":");
        char labourBuffer[24];
        snprintf(labourBuffer, sizeof(labourBuffer), "%llu",
                 static_cast<unsigned long long>(labour));
        json += labourBuffer;
        json += F(",\"client_price_minor\":");
        char clientBuffer[24];
        snprintf(clientBuffer, sizeof(clientBuffer), "%llu",
                 static_cast<unsigned long long>(clientPrice));
        json += clientBuffer;
        json += F(",\"currency\":\""); json += jsonEscape(currency);
        json += F("\",\"timestamp\":\""); json += jsonEscape(timestamp);
        json += F("\"}");
        ++pageCount;
        nextCursor = static_cast<uint32_t>(file.position());
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}
}
