#include "CM_WarehousePersistenceIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_WarehouseWriteOffRecord.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr uint8_t ReferenceBatchSize = 32U;

struct IdReference
{
    uint32_t id;
    uint8_t matches;

    IdReference() : id(0UL), matches(0U) {}
};

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

bool incrementRecordCount(uint32_t& recordCount)
{
    if (recordCount == 0xFFFFFFFFUL) return false;
    ++recordCount;
    return true;
}

bool resolveReferences(fs::FS& storage,
                       const char* path,
                       const char* key,
                       IdReference* references,
                       uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(path)) return false;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!findUnsigned(line, key, candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t index = 0U; index < count; ++index)
        {
            if (references[index].id != candidate) continue;
            if (references[index].matches == 0xFFU)
            {
                file.close();
                return false;
            }
            ++references[index].matches;
        }
    }
    file.close();

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (references[index].id == 0UL || references[index].matches != 1U)
            return false;
    }
    return true;
}

bool checkSpools(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    constexpr const char* Path = "/data/warehouse/spools.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousSpoolId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t spoolId = 0UL, diameter = 0UL, weight = 0UL;
        String status, wireType, optional;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL || spoolId <= previousSpoolId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) || diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) || status.length() == 0U ||
            (hasWireType && (!findString(line, "wire_type", wireType) ||
                             (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            return false;
        }
        previousSpoolId = spoolId;

        const char* optionalKeys[] = {"manufacturer", "supplier", "batch", "storage_location", "comment"};
        for (uint8_t i = 0U; i < sizeof(optionalKeys) / sizeof(optionalKeys[0]); ++i)
        {
            const String marker = String("\"") + optionalKeys[i] + F("\":");
            if (line.indexOf(marker) >= 0 && !findString(line, optionalKeys[i], optional))
            {
                file.close();
                return false;
            }
        }
        if (!incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

bool checkPrice(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    constexpr const char* Path = "/data/warehouse/price.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t price = 0UL;
        String currency;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

bool validateReferenceBatch(fs::FS& storage,
                            IdReference* spoolReferences,
                            uint8_t spoolCount,
                            IdReference* repairReferences,
                            uint8_t repairCount)
{
    constexpr const char* SpoolsPath = "/data/warehouse/spools.ndjson";
    constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    return resolveReferences(storage, RepairsPath, "repair_id",
                             repairReferences, repairCount) &&
           resolveReferences(storage, SpoolsPath, "spool_id",
                             spoolReferences, spoolCount);
}

bool checkMovementReferences(fs::FS& storage)
{
    constexpr const char* MovementsPath = "/data/warehouse/movements.ndjson";
    if (!storage.exists(MovementsPath)) return true;

    File file = storage.open(MovementsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    IdReference spoolReferences[ReferenceBatchSize];
    IdReference repairReferences[ReferenceBatchSize];
    uint8_t spoolCount = 0U;
    uint8_t repairCount = 0U;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        WarehouseWriteOffRecord record;
        if (!WarehouseWriteOffRecordCodec::parse(line, record))
        {
            file.close();
            return false;
        }

        repairReferences[repairCount].id = record.repairId;
        repairReferences[repairCount].matches = 0U;
        ++repairCount;

        if (record.hasSpoolId)
        {
            spoolReferences[spoolCount].id = record.spoolId;
            spoolReferences[spoolCount].matches = 0U;
            ++spoolCount;
        }

        if (repairCount == ReferenceBatchSize || spoolCount == ReferenceBatchSize)
        {
            if (!validateReferenceBatch(storage,
                                        spoolReferences, spoolCount,
                                        repairReferences, repairCount))
            {
                file.close();
                return false;
            }
            spoolCount = 0U;
            repairCount = 0U;
        }
    }

    if ((repairCount > 0U || spoolCount > 0U) &&
        !validateReferenceBatch(storage,
                                spoolReferences, spoolCount,
                                repairReferences, repairCount))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}
}

bool WarehousePersistenceIntegrityAudit::check(fs::FS& storage)
{
    WarehousePersistenceAuditMetrics ignoredMetrics;
    return check(storage, ignoredMetrics);
}

bool WarehousePersistenceIntegrityAudit::check(fs::FS& storage,
                                               WarehousePersistenceAuditMetrics& metrics)
{
    metrics = WarehousePersistenceAuditMetrics();
    uint32_t spoolRecordCount = 0UL;
    uint32_t priceRecordCount = 0UL;
    if (!checkSpools(storage, spoolRecordCount) ||
        !checkPrice(storage, priceRecordCount) ||
        !checkMovementReferences(storage))
    {
        return false;
    }
    metrics.spoolRecordCount = spoolRecordCount;
    metrics.priceRecordCount = priceRecordCount;
    return true;
}
}
