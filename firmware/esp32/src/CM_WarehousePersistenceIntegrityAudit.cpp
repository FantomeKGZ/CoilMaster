#include "CM_WarehousePersistenceIntegrityAudit.h"
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

bool checkSpools(fs::FS& storage)
{
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
        if (!line.startsWith("{") || !line.endsWith("}") ||
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
    }
    file.close();
    return true;
}

bool checkPrice(fs::FS& storage)
{
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
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U)
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
    return checkSpools(storage) && checkPrice(storage);
}
}
