#include "CM_WarehousePersistenceIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include <Arduino.h>

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

bool idExists(fs::FS& storage, const char* path, const char* key, uint32_t wanted)
{
    if (wanted == 0UL || !storage.exists(path)) return false;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL;
        if (!findUnsigned(line, key, id) || id == 0UL)
        {
            file.close();
            return false;
        }
        if (id == wanted && ++matches > 1U)
        {
            file.close();
            return false;
        }
    }
    file.close();
    return matches == 1U;
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

bool checkMovementReferences(fs::FS& storage)
{
    constexpr const char* MovementsPath = "/data/warehouse/movements.ndjson";
    constexpr const char* SpoolsPath = "/data/warehouse/spools.ndjson";
    constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    if (!storage.exists(MovementsPath)) return true;

    File file = storage.open(MovementsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t spoolId = 0UL, repairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !idExists(storage, SpoolsPath, "spool_id", spoolId) ||
            !idExists(storage, RepairsPath, "repair_id", repairId))
        {
            file.close();
            return false;
        }
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
