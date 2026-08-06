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
    if (closing < 0) return String();
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
    if (!m_ready || materialFilter == nullptr) return false;
    if (!m_storage.exists(SpoolsPath)) return true;

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t diameter = 0UL;
        String status;
        String wireType;
        if (!findUnsigned(line, "diameter_hundredths_mm", diameter)) continue;
        if (diameterHundredthsMm > 0U && diameter != diameterHundredthsMm) continue;
        findString(line, "status", status);
        if (status.length() > 0U && status != "ACTIVE") continue;
        findString(line, "wire_type", wireType);
        const char* material = classifyMaterial(wireType);
        if (!filterMatches(materialFilter, material)) continue;

        const String enriched = withMaterialClass(line, material);
        if (enriched.length() == 0U) continue;
        if (!first) json += ',';
        first = false;
        json += enriched;
        ++appendedCount;
    }
    file.close();
    return true;
}
}
