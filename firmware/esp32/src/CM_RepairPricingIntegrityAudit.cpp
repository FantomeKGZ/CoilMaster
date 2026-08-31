#include "CM_RepairPricingIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr uint8_t ReferenceBatchSize = 32U;
constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";

struct RepairReference
{
    uint32_t id;
    uint8_t matches;

    RepairReference() : id(0UL), matches(0U) {}
};

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;

    uint64_t parsed = 0ULL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool findUnsigned32(const String& line, const char* key, uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide);
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
        if (ch == '"')
            return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
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

bool resolveRepairReferences(fs::FS& storage,
                             RepairReference* references,
                             uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(RepairsPath)) return false;

    for (uint8_t index = 0U; index < count; ++index)
        references[index].matches = 0U;

    File file = storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentId = 0UL;
        // Workshop syntax is validated by its authoritative registry audit.
        // Keep this repeated pricing-reference scan focused on exact identity.
        if (!findUnsigned32(line, "repair_id", currentId) || currentId == 0UL)
        {
            file.close();
            return false;
        }

        for (uint8_t index = 0U; index < count; ++index)
        {
            RepairReference& reference = references[index];
            if (reference.id != currentId) continue;
            if (reference.matches == 0xFFU || ++reference.matches > 1U)
            {
                file.close();
                return false;
            }
        }
    }
    file.close();

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (references[index].matches != 1U) return false;
    }
    return true;
}
}

bool RepairPricingIntegrityAudit::check(fs::FS& storage)
{
    constexpr const char* Path = "/data/repairs/pricing.ndjson";
    if (!storage.exists(Path)) return true;

    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    RepairReference references[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t repairId = 0UL;
        uint64_t labour = 0ULL;
        uint64_t client = 0ULL;
        String currency, timestamp;
        if (!findUnsigned32(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", client) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            return false;
        }

        references[batchCount].id = repairId;
        references[batchCount].matches = 0U;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!resolveRepairReferences(storage, references, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U &&
        !resolveRepairReferences(storage, references, batchCount))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}
}
