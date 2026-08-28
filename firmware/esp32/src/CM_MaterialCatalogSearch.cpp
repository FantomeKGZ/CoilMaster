#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool containsSearch(String value, const String& search)
{
    if (search.length() == 0U) return true;
    value.toLowerCase();
    return value.indexOf(search) >= 0;
}
}

bool MaterialLedger::appendMaterialsPageJson(String& json,
                                             const String& searchSource,
                                             uint32_t cursor,
                                             uint8_t limit,
                                             uint16_t& count,
                                             uint32_t& nextCursor,
                                             bool& hasMore) const
{
    String search = searchSource;
    search.trim();
    if (search.length() == 0U)
        return appendMaterialsPageJson(json, cursor, limit, count, nextCursor, hasMore);
    if (search.length() > MaxSearchLength) return false;
    search.toLowerCase();

    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize) return false;
    if (!m_storage.exists(MaterialsPath)) return cursor == 0UL;

    File file = m_storage.open(MaterialsPath, FILE_READ);
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
    uint32_t previousId = 0UL;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t materialId = 0UL;
        uint32_t stock = 0UL;
        uint32_t price = 0UL;
        String name, unit, currency, status;
        if (!findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            materialId <= previousId ||
            !findString(line, "name", name) || name.length() == 0U ||
            !findString(line, "unit", unit) || unit.length() == 0U ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "status", status))
        {
            file.close();
            return false;
        }
        previousId = materialId;

        String comment;
        if (line.indexOf(F("\"comment\":")) >= 0 &&
            !findString(line, "comment", comment))
        {
            file.close();
            return false;
        }

        String wireType;
        uint32_t diameter = 0UL;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        const bool hasDiameter = line.indexOf(F("\"diameter_hundredths_mm\":")) >= 0;
        if (hasWireType != hasDiameter)
        {
            file.close();
            return false;
        }
        if (hasWireType &&
            (unit != "GRAM" ||
             !findString(line, "wire_type", wireType) ||
             (wireType != "CU" && wireType != "AL") ||
             !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
             diameter == 0UL || diameter > 65535UL))
        {
            file.close();
            return false;
        }

        if (status != "ACTIVE") continue;

        bool matches = containsSearch(name, search) ||
                       containsSearch(comment, search) ||
                       containsSearch(unit, search) ||
                       containsSearch(wireType, search);
        if (!matches && hasDiameter)
        {
            char diameterText[16];
            snprintf(diameterText, sizeof(diameterText), "%u.%02u",
                     static_cast<unsigned int>(diameter / 100UL),
                     static_cast<unsigned int>(diameter % 100UL));
            matches = String(diameterText).indexOf(search) >= 0;
        }
        if (!matches) continue;

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
