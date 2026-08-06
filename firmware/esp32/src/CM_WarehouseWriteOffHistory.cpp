#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::appendConfirmedWriteOffsJson(String& json,
                                                   uint32_t repairId,
                                                   uint16_t& appendedCount,
                                                   uint32_t& totalConsumedGrams) const
{
    appendedCount = 0U;
    totalConsumedGrams = 0UL;
    if (!m_ready || repairId == 0UL) return false;
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentRepairId = 0UL;
        uint32_t movementId = 0UL;
        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t before = 0UL;
        uint32_t after = 0UL;
        uint32_t mass = 0UL;
        uint32_t price = 0UL;
        String type;
        String status;
        String wireType;
        String currency;
        String timestamp;
        String comment;

        if (!findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status) || status != "CONFIRMED" ||
            !findUnsigned(line, "repair_id", currentRepairId) ||
            currentRepairId != repairId ||
            !findUnsigned(line, "movement_id", movementId) ||
            !findUnsigned(line, "spool_id", spoolId) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            !findUnsigned(line, "weight_before_g", before) ||
            !findUnsigned(line, "weight_after_g", after) ||
            !findUnsigned(line, "mass_g", mass))
        {
            continue;
        }

        findUnsigned(line, "price_per_kg_minor", price);
        findString(line, "wire_type", wireType);
        findString(line, "currency", currency);
        findString(line, "timestamp", timestamp);
        findString(line, "comment", comment);

        if (!first) json += ',';
        first = false;
        json += F("{\"movement_id\":"); json += movementId;
        json += F(",\"spool_id\":"); json += spoolId;
        json += F(",\"repair_id\":"); json += currentRepairId;
        json += F(",\"diameter_hundredths_mm\":"); json += diameter;
        json += F(",\"wire_type\":");
        if (wireType.length() > 0U)
        {
            json += '"'; json += jsonEscape(wireType); json += '"';
        }
        else json += F("null");
        json += F(",\"legacy_unknown_material\":");
        json += wireType.length() > 0U ? F("false") : F("true");
        json += F(",\"weight_before_g\":"); json += before;
        json += F(",\"weight_after_g\":"); json += after;
        json += F(",\"consumed_g\":"); json += mass;
        json += F(",\"price_per_kg_minor\":"); json += price;
        json += F(",\"currency\":\""); json += jsonEscape(currency.length() > 0U ? currency : String("KGS")); json += '"';
        json += F(",\"timestamp\":\""); json += jsonEscape(timestamp); json += '"';
        if (comment.length() > 0U)
        {
            json += F(",\"comment\":\""); json += jsonEscape(comment); json += '"';
        }
        json += '}';

        ++appendedCount;
        totalConsumedGrams += mass;
    }
    file.close();
    return true;
}
}
