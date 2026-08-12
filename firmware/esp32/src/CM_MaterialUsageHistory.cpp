#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool findUnsigned64Local(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    if (key == nullptr) return false;

    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int start = pos + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    if (start >= line.length() || !isDigit(line[start])) return false;
    if (line[start] == '0' && start + 1 < line.length() && isDigit(line[start + 1]))
        return false;

    uint64_t parsed = 0ULL;
    int end = start;
    while (end < line.length() && isDigit(line[end]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[end] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++end;
    }

    while (end < line.length() && line[end] == ' ') ++end;
    if (end >= line.length() || (line[end] != ',' && line[end] != '}')) return false;

    value = parsed;
    return true;
}
}

bool MaterialLedger::appendUsageHistoryPageJson(
    String& json,
    uint32_t repairId,
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
    if (!m_storage.exists(UsagePath)) return cursor == 0UL;

    File file = m_storage.open(UsagePath, FILE_READ);
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
    uint32_t previousUsageId = 0UL;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t usageId = 0UL;
        uint32_t currentRepairId = 0UL;
        uint32_t currentMaterialId = 0UL;
        uint32_t quantity = 0UL;
        uint32_t unitPrice = 0UL;
        uint64_t lineCost = 0ULL;
        String currency, timestamp;
        if (!findUnsigned(line, "usage_id", usageId) || usageId == 0UL ||
            usageId <= previousUsageId ||
            !findUnsigned(line, "repair_id", currentRepairId) ||
            currentRepairId == 0UL ||
            !findUnsigned(line, "material_id", currentMaterialId) ||
            currentMaterialId == 0UL ||
            !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", unitPrice) ||
            unitPrice == 0UL ||
            !findUnsigned64Local(line, "line_cost_minor", lineCost) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) ||
            timestamp.length() < 10U)
        {
            file.close();
            return false;
        }
        previousUsageId = usageId;

        const uint64_t product = static_cast<uint64_t>(quantity) *
                                 static_cast<uint64_t>(unitPrice);
        if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
            lineCost != (product + 500ULL) / 1000ULL)
        {
            file.close();
            return false;
        }

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }

        if (repairId > 0UL && currentRepairId != repairId) continue;
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
