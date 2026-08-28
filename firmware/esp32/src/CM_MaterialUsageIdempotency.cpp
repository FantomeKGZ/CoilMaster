#include "CM_MaterialUsageIdempotency.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1])) return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1])) return false;
    uint64_t parsed = 0ULL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"') return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}
}

bool MaterialUsageIdempotency::validOperationId(const String& operationId)
{
    if (operationId.length() < MinOperationIdLength ||
        operationId.length() > MaxOperationIdLength)
    {
        return false;
    }
    for (size_t index = 0U; index < operationId.length(); ++index)
    {
        const char ch = operationId[index];
        if (!isAlphaNumeric(ch) && ch != '-' && ch != '_') return false;
    }
    return true;
}

String MaterialUsageIdempotency::taggedComment(const String& operationId,
                                                const String& operatorComment)
{
    String tagged = String(F("MU_TX=")) + operationId + ';';
    if (operatorComment.length() > 0U)
    {
        tagged += ' ';
        tagged += operatorComment;
    }
    return tagged;
}

bool MaterialUsageIdempotency::lookup(fs::FS& storage,
                                      const String& operationId,
                                      uint32_t repairId,
                                      uint32_t materialId,
                                      uint32_t quantityMilli,
                                      MaterialUsageReplay& replay)
{
    replay = MaterialUsageReplay();
    if (!validOperationId(operationId) || repairId == 0UL ||
        materialId == 0UL || quantityMilli == 0UL)
    {
        return false;
    }
    if (!storage.exists(UsagePath)) return true;

    File file = storage.open(UsagePath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL ||
        (rawSize > 0U &&
         (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')) ||
        !file.seek(0U))
    {
        file.close();
        return false;
    }

    const String marker = String(F("MU_TX=")) + operationId + ';';
    uint32_t previousUsageId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t usageId = 0UL;
        uint32_t storedRepairId = 0UL;
        uint32_t storedMaterialId = 0UL;
        uint32_t storedQuantity = 0UL;
        uint32_t unitPrice = 0UL;
        uint64_t lineCost = 0ULL;
        String currency, timestamp;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "usage_id", usageId) || usageId == 0UL ||
            usageId <= previousUsageId ||
            !findUnsigned(line, "repair_id", storedRepairId) || storedRepairId == 0UL ||
            !findUnsigned(line, "material_id", storedMaterialId) || storedMaterialId == 0UL ||
            !findUnsigned(line, "quantity_milli", storedQuantity) || storedQuantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", unitPrice) || unitPrice == 0UL ||
            !findUnsigned64(line, "line_cost_minor", lineCost) ||
            !findString(line, "currency", currency) || currency != "KGS" ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            return false;
        }
        previousUsageId = usageId;

        const uint64_t product = static_cast<uint64_t>(storedQuantity) *
                                 static_cast<uint64_t>(unitPrice);
        if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
            lineCost != (product + 500ULL) / 1000ULL)
        {
            file.close();
            return false;
        }

        String comment;
        const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
        if (hasComment && !findString(line, "comment", comment))
        {
            file.close();
            return false;
        }
        if (!hasComment || comment.indexOf(marker) != 0) continue;

        if (replay.found)
        {
            file.close();
            return false;
        }
        replay.found = true;
        replay.usageId = usageId;
        replay.repairId = storedRepairId;
        replay.materialId = storedMaterialId;
        replay.quantityMilli = storedQuantity;
        replay.unitPriceMinor = unitPrice;
        replay.lineCostMinor = lineCost;
        replay.currency = currency;
        replay.payloadMatches = storedRepairId == repairId &&
                                storedMaterialId == materialId &&
                                storedQuantity == quantityMilli;
    }
    file.close();
    return true;
}
}
