#include "CM_MaterialUsageCorrectionIntegrityAudit.h"

#include <Arduino.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_MaterialUsageIdempotency.h"

namespace CM
{
namespace
{
constexpr const char* AdjustmentsPath = "/data/materials/adjustments.ndjson";
constexpr const char* UsagePath = "/data/materials/usage.ndjson";
constexpr uint8_t BatchSize = 16U;

struct CorrectionRef
{
    uint32_t sourceUsageId;
    uint32_t repairId;
    uint32_t materialId;
    uint32_t quantityMilli;
    uint64_t lineCostMinor;
    String operationId;
    uint64_t cumulativeQuantity;
    uint8_t operationMatches;
    uint8_t sourceMatches;

    CorrectionRef()
        : sourceUsageId(0UL), repairId(0UL), materialId(0UL), quantityMilli(0UL),
          lineCostMinor(0ULL), cumulativeQuantity(0ULL), operationMatches(0U),
          sourceMatches(0U) {}
};

bool hasField(const String& line, const char* key)
{
    return line.indexOf(String("\"") + key + F("\":")) >= 0;
}

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U])) return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t parsed = 0ULL;
    if (!findUnsigned64(line, key, parsed) || parsed > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (!escaped && ch == '"')
            return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (!escaped && ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (escaped)
        {
            if (ch == '"' || ch == '\\') value += ch;
            else if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else return false;
            escaped = false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool correctionShape(const String& line,
                     uint32_t& sourceUsageId,
                     uint32_t& repairId,
                     uint32_t& materialId,
                     uint32_t& quantityMilli,
                     uint64_t& lineCostMinor,
                     String& operationId,
                     bool& correctionRow)
{
    correctionRow = false;
    const bool hasSource = hasField(line, "correction_source_usage_id");
    const bool hasRepair = hasField(line, "correction_repair_id");
    const bool hasQuantity = hasField(line, "correction_quantity_milli");
    const bool hasCost = hasField(line, "correction_line_cost_minor");
    const bool hasOperation = hasField(line, "correction_operation_id");
    correctionRow = hasSource || hasRepair || hasQuantity || hasCost || hasOperation;
    if (!correctionRow) return true;
    if (!(hasSource && hasRepair && hasQuantity && hasCost && hasOperation)) return false;

    uint32_t added = 0UL, priceBefore = 0UL, priceAfter = 0UL;
    String type, status, currencyBefore, currencyAfter;
    if (!findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
        !findUnsigned(line, "added_quantity_milli", added) || added == 0UL ||
        !findUnsigned(line, "price_before_minor", priceBefore) || priceBefore == 0UL ||
        !findUnsigned(line, "price_after_minor", priceAfter) || priceAfter == 0UL ||
        !findString(line, "type", type) || type != "ADJUSTMENT" ||
        !findString(line, "status", status) || status != "CONFIRMED" ||
        !findString(line, "currency_before", currencyBefore) || currencyBefore != "KGS" ||
        !findString(line, "currency_after", currencyAfter) || currencyAfter != "KGS" ||
        !findUnsigned(line, "correction_source_usage_id", sourceUsageId) || sourceUsageId == 0UL ||
        !findUnsigned(line, "correction_repair_id", repairId) || repairId == 0UL ||
        !findUnsigned(line, "correction_quantity_milli", quantityMilli) || quantityMilli == 0UL ||
        !findUnsigned64(line, "correction_line_cost_minor", lineCostMinor) || lineCostMinor == 0ULL ||
        !findString(line, "correction_operation_id", operationId) ||
        !MaterialUsageIdempotency::validOperationId(operationId) ||
        quantityMilli != added || priceBefore != priceAfter)
    {
        return false;
    }
    return true;
}

bool resolveBatch(fs::FS& storage, CorrectionRef* refs, uint8_t count)
{
    if (count == 0U) return true;

    File adjustments = storage.open(AdjustmentsPath, FILE_READ);
    if (!adjustments || adjustments.isDirectory())
    {
        if (adjustments) adjustments.close();
        return false;
    }
    uint32_t previousAdjustmentId = 0UL;
    while (adjustments.available())
    {
        const String line = adjustments.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t adjustmentId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
            adjustmentId <= previousAdjustmentId)
        {
            adjustments.close();
            return false;
        }
        previousAdjustmentId = adjustmentId;

        uint32_t sourceUsageId = 0UL, repairId = 0UL, materialId = 0UL, quantity = 0UL;
        uint64_t cost = 0ULL;
        String operationId;
        bool correctionRow = false;
        if (!correctionShape(line, sourceUsageId, repairId, materialId, quantity,
                             cost, operationId, correctionRow))
        {
            adjustments.close();
            return false;
        }
        if (!correctionRow) continue;

        for (uint8_t i = 0U; i < count; ++i)
        {
            CorrectionRef& ref = refs[i];
            if (operationId == ref.operationId)
            {
                if (ref.operationMatches == 0xFFU || ++ref.operationMatches > 1U)
                {
                    adjustments.close();
                    return false;
                }
            }
            if (sourceUsageId == ref.sourceUsageId)
            {
                if (repairId != ref.repairId || materialId != ref.materialId ||
                    ref.cumulativeQuantity > 0xFFFFFFFFFFFFFFFFULL - quantity)
                {
                    adjustments.close();
                    return false;
                }
                ref.cumulativeQuantity += quantity;
            }
        }
    }
    adjustments.close();

    File usage = storage.open(UsagePath, FILE_READ);
    if (!usage || usage.isDirectory())
    {
        if (usage) usage.close();
        return false;
    }
    uint32_t previousUsageId = 0UL;
    while (usage.available())
    {
        const String line = usage.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t usageId = 0UL, repairId = 0UL, materialId = 0UL;
        uint32_t quantity = 0UL, price = 0UL;
        uint64_t lineCost = 0ULL;
        String currency, timestamp, comment;
        const bool hasComment = hasField(line, "comment");
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "usage_id", usageId) || usageId == 0UL || usageId <= previousUsageId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findUnsigned64(line, "line_cost_minor", lineCost) ||
            !findString(line, "currency", currency) || currency != "KGS" ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            (hasComment && !findString(line, "comment", comment)))
        {
            usage.close();
            return false;
        }
        previousUsageId = usageId;
        const uint64_t product = static_cast<uint64_t>(quantity) * price;
        if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
            lineCost != (product + 500ULL) / 1000ULL)
        {
            usage.close();
            return false;
        }

        for (uint8_t i = 0U; i < count; ++i)
        {
            CorrectionRef& ref = refs[i];
            if (usageId != ref.sourceUsageId) continue;
            if (ref.sourceMatches == 0xFFU || ++ref.sourceMatches > 1U ||
                repairId != ref.repairId || materialId != ref.materialId ||
                (hasComment && comment.indexOf(F("RWI_TX=")) == 0) ||
                ref.cumulativeQuantity > quantity)
            {
                usage.close();
                return false;
            }
            const uint64_t correctionProduct = static_cast<uint64_t>(ref.quantityMilli) * price;
            if (correctionProduct > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
                ref.lineCostMinor != (correctionProduct + 500ULL) / 1000ULL ||
                ref.lineCostMinor > lineCost)
            {
                usage.close();
                return false;
            }
        }
    }
    usage.close();

    for (uint8_t i = 0U; i < count; ++i)
    {
        if (refs[i].operationMatches != 1U || refs[i].sourceMatches != 1U)
            return false;
    }
    return true;
}
}

bool MaterialUsageCorrectionIntegrityAudit::check(fs::FS& storage)
{
    if (!storage.exists(AdjustmentsPath)) return true;
    if (!storage.exists(UsagePath))
    {
        File adjustments = storage.open(AdjustmentsPath, FILE_READ);
        if (!adjustments || adjustments.isDirectory())
        {
            if (adjustments) adjustments.close();
            return false;
        }
        while (adjustments.available())
        {
            const String line = adjustments.readStringUntil('\n');
            if (line.length() == 0U) continue;
            if (!FlatJsonObjectValidator::valid(line))
            {
                adjustments.close();
                return false;
            }
            if (hasField(line, "correction_source_usage_id") ||
                hasField(line, "correction_repair_id") ||
                hasField(line, "correction_quantity_milli") ||
                hasField(line, "correction_line_cost_minor") ||
                hasField(line, "correction_operation_id"))
            {
                adjustments.close();
                return false;
            }
        }
        adjustments.close();
        return true;
    }

    File file = storage.open(AdjustmentsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    CorrectionRef refs[BatchSize];
    uint8_t count = 0U;
    uint32_t previousAdjustmentId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t adjustmentId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
            adjustmentId <= previousAdjustmentId)
        {
            file.close();
            return false;
        }
        previousAdjustmentId = adjustmentId;

        uint32_t sourceUsageId = 0UL, repairId = 0UL, materialId = 0UL, quantity = 0UL;
        uint64_t cost = 0ULL;
        String operationId;
        bool correctionRow = false;
        if (!correctionShape(line, sourceUsageId, repairId, materialId, quantity,
                             cost, operationId, correctionRow))
        {
            file.close();
            return false;
        }
        if (!correctionRow) continue;

        refs[count] = CorrectionRef();
        refs[count].sourceUsageId = sourceUsageId;
        refs[count].repairId = repairId;
        refs[count].materialId = materialId;
        refs[count].quantityMilli = quantity;
        refs[count].lineCostMinor = cost;
        refs[count].operationId = operationId;
        ++count;
        if (count == BatchSize)
        {
            if (!resolveBatch(storage, refs, count))
            {
                file.close();
                return false;
            }
            count = 0U;
        }
    }
    file.close();

    return count == 0U || resolveBatch(storage, refs, count);
}
}
