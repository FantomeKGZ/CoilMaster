#include "CM_RepairCosting.h"

namespace CM
{
bool RepairCosting::appendPricingRevisionsJson(String& json,
                                               uint32_t repairId,
                                               uint16_t& appendedCount,
                                               PricingRevisionSnapshot& latest) const
{
    appendedCount = 0U;
    latest = PricingRevisionSnapshot();
    if (!ready() || repairId == 0UL) return false;
    if (!m_storage.exists(PricingPath)) return true;

    File file = m_storage.open(PricingPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t lineRepairId = 0UL;
        uint64_t labour = 0ULL;
        uint64_t clientPrice = 0ULL;
        String currency;
        String timestamp;

        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "repair_id", lineRepairId) || lineRepairId == 0UL ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", clientPrice) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            appendedCount = 0U;
            latest = PricingRevisionSnapshot();
            return false;
        }

        if (lineRepairId != repairId) continue;
        if (appendedCount == 0xFFFFU)
        {
            file.close();
            appendedCount = 0U;
            latest = PricingRevisionSnapshot();
            return false;
        }

        if (!first) json += ',';
        first = false;
        json += F("{\"revision\":");
        json += static_cast<uint32_t>(appendedCount) + 1UL;
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
        json += F(",\"currency\":\"");
        json += jsonEscape(currency);
        json += F("\",\"timestamp\":\"");
        json += jsonEscape(timestamp);
        json += F("\"}");

        latest.labourCostMinor = labour;
        latest.clientPriceMinor = clientPrice;
        latest.currency = currency;
        latest.timestamp = timestamp;
        ++appendedCount;
    }
    file.close();
    return true;
}
}
