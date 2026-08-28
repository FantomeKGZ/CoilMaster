#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_MaterialUsageIdempotency.h"

namespace CM
{
namespace
{
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

bool hasField(const String& line, const char* key)
{
    return line.indexOf(String("\"") + key + F("\":")) >= 0;
}
}

bool MaterialLedger::correctUsage(const MaterialUsageCorrection& correction,
                                  MaterialUsageCorrectionResult& result)
{
    result = MaterialUsageCorrectionResult();
    result.sourceUsageId = correction.sourceUsageId;
    result.repairId = correction.repairId;
    result.correctedQuantityMilli = correction.quantityMilli;

    if (!ready() || correction.sourceUsageId == 0UL || correction.repairId == 0UL ||
        correction.quantityMilli == 0UL || correction.timestamp.length() < 10U ||
        !MaterialUsageIdempotency::validOperationId(correction.operationId) ||
        !m_storage.exists(UsagePath) || m_storage.exists(AdjustmentPendingPath))
    {
        return false;
    }

    uint32_t sourceRepairId = 0UL;
    uint32_t sourceMaterialId = 0UL;
    uint32_t sourceQuantity = 0UL;
    uint32_t sourcePrice = 0UL;
    uint64_t sourceLineCost = 0ULL;
    String sourceCurrency;
    String sourceComment;
    bool sourceFound = false;

    File usage = m_storage.open(UsagePath, FILE_READ);
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
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "usage_id", usageId) || usageId == 0UL || usageId <= previousUsageId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findUnsigned64(line, "line_cost_minor", lineCost) ||
            !findString(line, "currency", currency) || currency != "KGS" ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            usage.close();
            return false;
        }
        previousUsageId = usageId;
        const uint64_t product = static_cast<uint64_t>(quantity) * static_cast<uint64_t>(price);
        if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
            lineCost != (product + 500ULL) / 1000ULL)
        {
            usage.close();
            return false;
        }
        if (hasField(line, "comment") && !findString(line, "comment", comment))
        {
            usage.close();
            return false;
        }
        if (usageId != correction.sourceUsageId) continue;
        if (sourceFound)
        {
            usage.close();
            return false;
        }
        sourceFound = true;
        sourceRepairId = repairId;
        sourceMaterialId = materialId;
        sourceQuantity = quantity;
        sourcePrice = price;
        sourceLineCost = lineCost;
        sourceCurrency = currency;
        sourceComment = comment;
    }
    usage.close();

    if (!sourceFound)
    {
        result.status = MaterialUsageCorrectionStatus::SourceNotFound;
        return true;
    }
    result.materialId = sourceMaterialId;
    result.unitPriceMinor = sourcePrice;
    result.currency = sourceCurrency;
    if (sourceRepairId != correction.repairId)
    {
        result.status = MaterialUsageCorrectionStatus::SourceRepairMismatch;
        return true;
    }
    if (sourceComment.indexOf(F("RWI_TX=")) == 0)
    {
        result.status = MaterialUsageCorrectionStatus::RunWireForbidden;
        return true;
    }

    uint64_t correctedTotal = 0ULL;
    bool operationFound = false;
    bool operationPayloadMatches = false;
    uint32_t operationAdjustmentId = 0UL;
    uint32_t operationQuantity = 0UL;
    uint64_t operationLineCost = 0ULL;

    if (m_storage.exists(AdjustmentsPath))
    {
        File adjustments = m_storage.open(AdjustmentsPath, FILE_READ);
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
            uint32_t adjustmentId = 0UL, materialId = 0UL, added = 0UL;
            uint32_t stockBefore = 0UL, stockAfter = 0UL;
            uint32_t priceBefore = 0UL, priceAfter = 0UL;
            String type, status, currencyBefore, currencyAfter, timestamp;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
                adjustmentId <= previousAdjustmentId ||
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
                adjustments.close();
                return false;
            }
            previousAdjustmentId = adjustmentId;

            const bool hasSource = hasField(line, "correction_source_usage_id");
            const bool hasRepair = hasField(line, "correction_repair_id");
            const bool hasQuantity = hasField(line, "correction_quantity_milli");
            const bool hasCost = hasField(line, "correction_line_cost_minor");
            const bool hasOperation = hasField(line, "correction_operation_id");
            const bool correctionRow = hasSource || hasRepair || hasQuantity || hasCost || hasOperation;
            if (!correctionRow) continue;
            if (!(hasSource && hasRepair && hasQuantity && hasCost && hasOperation))
            {
                adjustments.close();
                return false;
            }

            uint32_t correctionSource = 0UL, correctionRepair = 0UL, correctionQuantity = 0UL;
            uint64_t correctionCost = 0ULL;
            String operationId;
            if (!findUnsigned(line, "correction_source_usage_id", correctionSource) || correctionSource == 0UL ||
                !findUnsigned(line, "correction_repair_id", correctionRepair) || correctionRepair == 0UL ||
                !findUnsigned(line, "correction_quantity_milli", correctionQuantity) || correctionQuantity == 0UL ||
                !findUnsigned64(line, "correction_line_cost_minor", correctionCost) || correctionCost == 0ULL ||
                !findString(line, "correction_operation_id", operationId) ||
                !MaterialUsageIdempotency::validOperationId(operationId) ||
                correctionQuantity != added || priceBefore != priceAfter)
            {
                adjustments.close();
                return false;
            }

            if (correctionSource == correction.sourceUsageId)
            {
                if (materialId != sourceMaterialId || correctionRepair != sourceRepairId ||
                    correctedTotal > 0xFFFFFFFFFFFFFFFFULL - correctionQuantity)
                {
                    adjustments.close();
                    return false;
                }
                correctedTotal += correctionQuantity;
                if (correctedTotal > sourceQuantity)
                {
                    adjustments.close();
                    return false;
                }
            }

            if (operationId == correction.operationId)
            {
                if (operationFound)
                {
                    adjustments.close();
                    return false;
                }
                operationFound = true;
                operationAdjustmentId = adjustmentId;
                operationQuantity = correctionQuantity;
                operationLineCost = correctionCost;
                operationPayloadMatches = correctionSource == correction.sourceUsageId &&
                                          correctionRepair == correction.repairId &&
                                          materialId == sourceMaterialId &&
                                          correctionQuantity == correction.quantityMilli;
            }
        }
        adjustments.close();
    }

    if (operationFound)
    {
        if (!operationPayloadMatches)
        {
            result.status = MaterialUsageCorrectionStatus::OperationIdConflict;
            return true;
        }
        const uint64_t expectedCost =
            (static_cast<uint64_t>(operationQuantity) * sourcePrice + 500ULL) / 1000ULL;
        if (operationLineCost != expectedCost || correctedTotal > sourceQuantity)
            return false;
        MaterialItemState current;
        bool currentFound = false;
        if (!loadActiveMaterialState(sourceMaterialId, current, currentFound) || !currentFound ||
            current.currency != sourceCurrency)
            return false;
        result.adjustmentId = operationAdjustmentId;
        result.correctionLineCostMinor = operationLineCost;
        result.stockQuantityMilli = current.stockQuantityMilli;
        result.remainingCorrectableQuantityMilli = sourceQuantity - static_cast<uint32_t>(correctedTotal);
        result.status = MaterialUsageCorrectionStatus::DuplicateReplay;
        return true;
    }

    if (correctedTotal > sourceQuantity ||
        correction.quantityMilli > sourceQuantity - static_cast<uint32_t>(correctedTotal))
    {
        result.remainingCorrectableQuantityMilli =
            sourceQuantity - static_cast<uint32_t>(correctedTotal);
        result.status = MaterialUsageCorrectionStatus::OverCorrection;
        return true;
    }

    const uint64_t correctionCost =
        (static_cast<uint64_t>(correction.quantityMilli) * sourcePrice + 500ULL) / 1000ULL;
    if (correctionCost == 0ULL || correctionCost > sourceLineCost) return false;

    MaterialAdjustment adjustment;
    adjustment.materialId = sourceMaterialId;
    adjustment.addQuantityMilli = correction.quantityMilli;
    adjustment.newPricePerUnitMinor = 0UL;
    adjustment.currency = sourceCurrency;
    adjustment.timestamp = correction.timestamp;
    adjustment.comment = correction.comment;
    adjustment.correctionSourceUsageId = correction.sourceUsageId;
    adjustment.correctionRepairId = correction.repairId;
    adjustment.correctionQuantityMilli = correction.quantityMilli;
    adjustment.correctionLineCostMinor = correctionCost;
    adjustment.correctionOperationId = correction.operationId;

    MaterialAdjustmentResult adjustmentResult;
    if (!adjustMaterial(adjustment, adjustmentResult)) return false;

    result.adjustmentId = adjustmentResult.adjustmentId;
    result.stockQuantityMilli = adjustmentResult.stockQuantityMilli;
    result.correctionLineCostMinor = correctionCost;
    result.remainingCorrectableQuantityMilli =
        sourceQuantity - static_cast<uint32_t>(correctedTotal) - correction.quantityMilli;
    result.status = MaterialUsageCorrectionStatus::Confirmed;
    return true;
}
}
