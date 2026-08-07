#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::appendActiveSpoolsJson(String& json,
                                             uint16_t diameterHundredthsMm,
                                             uint16_t& appendedCount) const
{
    appendedCount = 0U;
    if (!ready()) return false;
    if (!m_storage.exists(SpoolsPath)) return true;

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;

        if (!findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status))
        {
            file.close();
            return false;
        }
        previousId = spoolId;

        String wireType;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (hasWireType &&
            (!findString(line, "wire_type", wireType) ||
             (wireType != "CU" && wireType != "AL")))
        {
            file.close();
            return false;
        }

        const char* optionalFields[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        String optionalValues[5];
        bool optionalPresent[5] = {false, false, false, false, false};
        for (uint8_t i = 0U; i < 5U; ++i)
        {
            const String marker = String("\"") + optionalFields[i] + F("\":");
            optionalPresent[i] = line.indexOf(marker) >= 0;
            if (optionalPresent[i] && !findString(line, optionalFields[i], optionalValues[i]))
            {
                file.close();
                return false;
            }
        }

        if (status != "ACTIVE") continue;
        if (diameterHundredthsMm > 0U && diameter != diameterHundredthsMm) continue;
        if (appendedCount == 0xFFFFU)
        {
            file.close();
            return false;
        }

        if (!first) json += ',';
        first = false;

        json += F("{\"spool_id\":"); json += spoolId;
        json += F(",\"diameter_hundredths_mm\":"); json += diameter;
        json += F(",\"current_weight_g\":"); json += weight;
        json += F(",\"status\":\"ACTIVE\"");
        if (hasWireType)
        {
            json += F(",\"wire_type\":\""); json += jsonEscape(wireType); json += '"';
        }
        for (uint8_t i = 0U; i < 5U; ++i)
        {
            if (!optionalPresent[i]) continue;
            json += F(",\""); json += optionalFields[i]; json += F("\":\"");
            json += jsonEscape(optionalValues[i]); json += '"';
        }
        json += '}';
        ++appendedCount;
    }

    file.close();
    return true;
}
}
