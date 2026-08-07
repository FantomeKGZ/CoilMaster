#include "CM_WarehouseStore.h"

namespace CM
{
namespace
{
const char* classifyMaterial(const String& wireType)
{
    if (wireType == "CU") return "CU";
    if (wireType == "AL") return "AL";
    return "UNKNOWN";
}

bool filterMatches(const char* filter, const char* material)
{
    if (filter == nullptr || strcmp(filter, "ALL") == 0) return true;
    return strcmp(filter, material) == 0;
}

String withMaterialClass(const String& source, const char* material)
{
    String line = source;
    line.trim();
    const int closing = line.lastIndexOf('}');
    if (closing < 0 || closing != line.length() - 1) return String();
    String result;
    result.reserve(line.length() + 72U);
    result = line.substring(0, closing);
    result += F(",\"material_class\":\"");
    result += material;
    result += '"';
    if (strcmp(material, "UNKNOWN") == 0)
        result += F(",\"legacy_unknown_material\":true");
    result += '}';
    return result;
}
}

bool WarehouseStore::appendActiveSpoolsJson(String& json,
                                            uint16_t diameterHundredthsMm,
                                            const char* materialFilter,
                                            uint16_t& appendedCount) const
{
    appendedCount = 0U;
    if (!ready() || materialFilter == nullptr) return false;
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
        String status, wireType;
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
        for (uint8_t i = 0U; i < 5U; ++i)
        {
            const String marker = String("\"") + optionalFields[i] + F("\":");
            if (line.indexOf(marker) >= 0)
            {
                String value;
                if (!findString(line, optionalFields[i], value))
                {
                    file.close();
                    return false;
                }
            }
        }

        if (status != "ACTIVE") continue;
        if (diameterHundredthsMm > 0U && diameter != diameterHundredthsMm) continue;

        const char* material = classifyMaterial(wireType);
        if (!filterMatches(materialFilter, material)) continue;

        const String enriched = withMaterialClass(line, material);
        if (enriched.length() == 0U || appendedCount == 0xFFFFU)
        {
            file.close();
            return false;
        }
        if (!first) json += ',';
        first = false;
        json += enriched;
        ++appendedCount;
    }
    file.close();
    return true;
}
}
