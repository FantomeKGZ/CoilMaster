#include "CM_RepairCosting.h"

namespace CM
{
bool RepairCosting::appendPricingRevisionsJson(String& json,
                                               uint32_t repairId,
                                               uint16_t& appendedCount) const
{
    appendedCount = 0U;
    if (!m_ready || repairId == 0UL) return false;
    if (!m_storage.exists(PricingPath)) return true;

    File file = m_storage.open(PricingPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t lineRepairId = 0UL;
        uint64_t labour = 0ULL;
        uint64_t clientPrice = 0ULL;
        String currency;
        String timestamp;

        if (!findUnsigned(line, "repair_id", lineRepairId) ||
            lineRepairId != repairId ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", clientPrice))
        {
            continue;
        }

        findString(line, "currency", currency);
        findString(line, "timestamp", timestamp);

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
        json += jsonEscape(currency.length() == 3U ? currency : String("KGS"));
        json += F("\",\"timestamp\":\"");
        json += jsonEscape(timestamp);
        json += F("\"}");

        if (appendedCount < 0xFFFFU) ++appendedCount;
    }
    file.close();
    return true;
}
}
