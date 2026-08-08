#include "CM_MaterialPersistenceIntegrityAudit.h"
#include "CM_WorkshopPersistenceIntegrityAudit.h"
#include "CM_RepairPricingIntegrityAudit.h"
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

bool validLineShape(const String& line)
{
    return line.length() > 1U && line.startsWith("{") && line.endsWith("}");
}

bool validMaterialUnit(const String& unit)
{
    return unit == "PIECE" || unit == "GRAM" || unit == "MILLILITRE" ||
           unit == "METRE" || unit == "SQUARE_METRE";
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

bool checkMaterials(fs::FS& storage)
{
    constexpr const char* Path = "/data/materials/materials.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t materialId = 0UL, stock = 0UL, price = 0UL;
        String name, unit, currency, status;
        if (!validLineShape(line) ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL || materialId <= previousId ||
            !findString(line, "name", name) || name.length() == 0U ||
            !findString(line, "unit", unit) || !validMaterialUnit(unit) ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency != "KGS" ||
            !findString(line, "status", status) || status.length() == 0U)
        {
            file.close();
            return false;
        }
        previousId = materialId;
        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }
    }
    file.close();
    return true;
}

bool checkUsage(fs::FS& storage)
{
    constexpr const char* Path = "/data/materials/usage.ndjson";
    constexpr const char* MaterialsPath = "/data/materials/materials.ndjson";
    constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t usageId = 0UL, repairId = 0UL, materialId = 0UL;
        uint32_t quantity = 0UL, unitPrice = 0UL;
        uint64_t lineCost = 0ULL;
        String currency, timestamp;
        if (!validLineShape(line) ||
            !findUnsigned(line, "usage_id", usageId) || usageId == 0UL || usageId <= previousId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", unitPrice) || unitPrice == 0UL ||
            !findUnsigned64(line, "line_cost_minor", lineCost) ||
            !findString(line, "currency", currency) || currency != "KGS" ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            !idExists(storage, MaterialsPath, "material_id", materialId) ||
            !idExists(storage, RepairsPath, "repair_id", repairId))
        {
            file.close();
            return false;
        }
        previousId = usageId;
        const uint64_t product = static_cast<uint64_t>(quantity) * static_cast<uint64_t>(unitPrice);
        if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL || lineCost != (product + 500ULL) / 1000ULL)
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
    }
    file.close();
    return true;
}

bool checkAdjustments(fs::FS& storage)
{
    constexpr const char* Path = "/data/materials/adjustments.ndjson";
    constexpr const char* MaterialsPath = "/data/materials/materials.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t adjustmentId = 0UL, materialId = 0UL, added = 0UL;
        uint32_t stockBefore = 0UL, stockAfter = 0UL, priceBefore = 0UL, priceAfter = 0UL;
        String type, status, currencyBefore, currencyAfter, timestamp;
        if (!validLineShape(line) ||
            !findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL || adjustmentId <= previousId ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            !findString(line, "type", type) || type != "ADJUSTMENT" ||
            !findString(line, "status", status) || status != "CONFIRMED" ||
            !findUnsigned(line, "added_quantity_milli", added) ||
            !findUnsigned(line, "stock_before_milli", stockBefore) ||
            !findUnsigned(line, "stock_after_milli", stockAfter) ||
            stockBefore > 0xFFFFFFFFUL - added || stockAfter != stockBefore + added ||
            !findUnsigned(line, "price_before_minor", priceBefore) || priceBefore == 0UL ||
            !findUnsigned(line, "price_after_minor", priceAfter) || priceAfter == 0UL ||
            !findString(line, "currency_before", currencyBefore) || currencyBefore != "KGS" ||
            !findString(line, "currency_after", currencyAfter) || currencyAfter != "KGS" ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            !idExists(storage, MaterialsPath, "material_id", materialId))
        {
            file.close();
            return false;
        }
        previousId = adjustmentId;
        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }
    }
    file.close();
    return true;
}
}

bool MaterialPersistenceIntegrityAudit::check(fs::FS& storage)
{
    return checkMaterials(storage) &&
           checkUsage(storage) &&
           checkAdjustments(storage) &&
           WorkshopPersistenceIntegrityAudit::check(storage) &&
           RepairPricingIntegrityAudit::check(storage);
}
}
