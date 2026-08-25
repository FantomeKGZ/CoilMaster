#include "CM_MaterialPersistenceIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_WorkshopPersistenceIntegrityAudit.h"
#include "CM_RepairPricingIntegrityAudit.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr uint8_t ReferenceBatchSize = 32U;

struct ExactIdReference
{
    uint32_t id;
    uint8_t matches;

    ExactIdReference() : id(0UL), matches(0U) {}
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
    return FlatJsonObjectValidator::valid(line);
}

bool validMaterialUnit(const String& unit)
{
    return unit == "PIECE" || unit == "GRAM" || unit == "MILLILITRE" ||
           unit == "METRE" || unit == "SQUARE_METRE";
}

bool resolveExactReferences(fs::FS& storage,
                            const char* path,
                            const char* key,
                            ExactIdReference* references,
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

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (references[index].id == 0UL)
        {
            file.close();
            return false;
        }
        references[index].matches = 0U;
    }

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
        for (uint8_t index = 0U; index < count; ++index)
        {
            ExactIdReference& reference = references[index];
            if (reference.id != id) continue;
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

bool checkMaterials(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
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
        if (recordCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++recordCount;
    }
    file.close();
    return true;
}

bool checkUsage(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
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

    ExactIdReference materialReferences[ReferenceBatchSize];
    ExactIdReference repairReferences[ReferenceBatchSize];
    uint8_t batchCount = 0U;
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
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
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
        if (recordCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++recordCount;

        materialReferences[batchCount].id = materialId;
        materialReferences[batchCount].matches = 0U;
        repairReferences[batchCount].id = repairId;
        repairReferences[batchCount].matches = 0U;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!resolveExactReferences(storage, MaterialsPath, "material_id",
                                        materialReferences, batchCount) ||
                !resolveExactReferences(storage, RepairsPath, "repair_id",
                                        repairReferences, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U &&
        (!resolveExactReferences(storage, MaterialsPath, "material_id",
                                 materialReferences, batchCount) ||
         !resolveExactReferences(storage, RepairsPath, "repair_id",
                                 repairReferences, batchCount)))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool checkAdjustments(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    constexpr const char* Path = "/data/materials/adjustments.ndjson";
    constexpr const char* MaterialsPath = "/data/materials/materials.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    ExactIdReference materialReferences[ReferenceBatchSize];
    uint8_t batchCount = 0U;
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
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
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
        if (recordCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++recordCount;

        materialReferences[batchCount].id = materialId;
        materialReferences[batchCount].matches = 0U;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!resolveExactReferences(storage, MaterialsPath, "material_id",
                                        materialReferences, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U &&
        !resolveExactReferences(storage, MaterialsPath, "material_id",
                                materialReferences, batchCount))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}
}

bool MaterialPersistenceIntegrityAudit::check(fs::FS& storage)
{
    MaterialPersistenceAuditMetrics ignoredMetrics;
    if (!checkMaterialDomain(storage, ignoredMetrics)) return false;

    // Standalone callers retain the original broad integrity contract.
    return WorkshopPersistenceIntegrityAudit::check(storage) &&
           RepairPricingIntegrityAudit::check(storage);
}

bool MaterialPersistenceIntegrityAudit::checkMaterialDomain(
    fs::FS& storage,
    MaterialPersistenceAuditMetrics& metrics)
{
    metrics.materialRecordCount = 0UL;
    metrics.usageRecordCount = 0UL;
    metrics.adjustmentRecordCount = 0UL;

    uint32_t materialRecordCount = 0UL;
    uint32_t usageRecordCount = 0UL;
    uint32_t adjustmentRecordCount = 0UL;
    if (!checkMaterials(storage, materialRecordCount) ||
        !checkUsage(storage, usageRecordCount) ||
        !checkAdjustments(storage, adjustmentRecordCount))
    {
        return false;
    }

    metrics.materialRecordCount = materialRecordCount;
    metrics.usageRecordCount = usageRecordCount;
    metrics.adjustmentRecordCount = adjustmentRecordCount;
    return true;
}

bool MaterialPersistenceIntegrityAudit::check(fs::FS& storage,
                                              MaterialPersistenceAuditMetrics& metrics)
{
    // Metrics overload is used by the composite backup audit, which runs the
    // authoritative business/pricing domain immediately afterwards.
    return checkMaterialDomain(storage, metrics);
}
}
