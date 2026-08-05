#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::appendActiveSpoolsJson(String& json,
                                             uint16_t diameterHundredthsMm,
                                             uint16_t& appendedCount) const
{
    appendedCount = 0U;
    if (!m_ready)
    {
        return false;
    }

    if (!m_storage.exists(SpoolsPath))
    {
        return true;
    }

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file)
    {
        return false;
    }

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;

        if (!findUnsigned(line, "spool_id", spoolId) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            !findUnsigned(line, "current_weight_g", weight))
        {
            continue;
        }

        findString(line, "status", status);
        if (status.length() > 0U && status != "ACTIVE")
        {
            continue;
        }

        if (diameterHundredthsMm > 0U && diameter != diameterHundredthsMm)
        {
            continue;
        }

        if (!first) json += ',';
        first = false;

        json += F("{\"spool_id\":"); json += spoolId;
        json += F(",\"diameter_hundredths_mm\":"); json += diameter;
        json += F(",\"current_weight_g\":"); json += weight;
        json += F(",\"status\":\"ACTIVE\"");

        String value;
        if (findString(line, "wire_type", value))
        {
            json += F(",\"wire_type\":\""); json += jsonEscape(value); json += '"';
        }
        if (findString(line, "manufacturer", value))
        {
            json += F(",\"manufacturer\":\""); json += jsonEscape(value); json += '"';
        }
        if (findString(line, "supplier", value))
        {
            json += F(",\"supplier\":\""); json += jsonEscape(value); json += '"';
        }
        if (findString(line, "batch", value))
        {
            json += F(",\"batch\":\""); json += jsonEscape(value); json += '"';
        }
        if (findString(line, "storage_location", value))
        {
            json += F(",\"storage_location\":\""); json += jsonEscape(value); json += '"';
        }
        if (findString(line, "comment", value))
        {
            json += F(",\"comment\":\""); json += jsonEscape(value); json += '"';
        }

        json += '}';
        if (appendedCount < 0xFFFFU) ++appendedCount;
    }

    file.close();
    return true;
}
}
