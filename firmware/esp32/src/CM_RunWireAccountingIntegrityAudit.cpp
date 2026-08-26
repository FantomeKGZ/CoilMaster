#include "CM_RunWireAccountingIntegrityAudit.h"

#include <Arduino.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_MaterialRequestMovementStore.h"
#include "CM_RunWireIssuePendingStore.h"
#include "CM_SpoolMaterialBridgeStore.h"
#include "CM_WarehouseWriteOffRecord.h"

namespace CM
{
namespace
{
constexpr uint8_t ReferenceBatchSize = 16U;
constexpr const char* UsagePath = "/data/materials/usage.ndjson";
constexpr const char* WarehouseMovementsPath = "/data/warehouse/movements.ndjson";

struct RunWireReference
{
    String transactionRef;
    uint32_t repairId;
    uint32_t warehouseItemId;
    uint32_t sourceSessionId;
    uint32_t sourceRunId;
    bool hasPersistedSpoolId;
    uint32_t persistedSpoolId;
    uint32_t spoolId;
    uint32_t consumedGrams;
    uint32_t ledgerQuantityMilli;
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;
    String materialClass;
    uint16_t diameterHundredthsMm;
    String createdAt;
    uint8_t ledgerMatches;
    uint8_t warehouseMatches;

    RunWireReference()
        : repairId(0UL), warehouseItemId(0UL), sourceSessionId(0UL),
          sourceRunId(0UL), hasPersistedSpoolId(false), persistedSpoolId(0UL),
          spoolId(0UL), consumedGrams(0UL), ledgerQuantityMilli(0UL),
          unitCostMinor(0ULL), costAmountMinor(0ULL), diameterHundredthsMm(0U),
          ledgerMatches(0U), warehouseMatches(0U)
    {
    }
};

bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U]))
        return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (UINT64_MAX - digit) / 10ULL) return false;
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
    const String marker = String('"') + key + F("\":\"");
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

bool extractRunWireTag(const String& comment, String& transactionRef, bool& tagged)
{
    transactionRef = String();
    tagged = comment.indexOf(F("RWI_TX=")) == 0;
    if (!tagged) return true;
    const int separator = comment.indexOf(';', 7);
    if (separator < 0) return false;
    transactionRef = comment.substring(7, separator);
    return transactionRef.length() >= 8U && transactionRef.length() <= 80U &&
           transactionRef.indexOf(F("RWI-")) == 0;
}

bool bridgeMatches(fs::FS& storage, const RunWireReference& reference)
{
    if (!storage.exists(SpoolMaterialBridgeStore::Path)) return false;
    File file = storage.open(SpoolMaterialBridgeStore::Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t spoolId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL)
        {
            file.close();
            return false;
        }
        if (spoolId != reference.spoolId) continue;

        uint32_t itemId = 0UL, diameter = 0UL;
        String wireType;
        if (++matches > 1U ||
            !findUnsigned(line, "warehouse_item_id", itemId) ||
            !findString(line, "wire_type", wireType) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            itemId != reference.warehouseItemId ||
            wireType != reference.materialClass ||
            diameter != reference.diameterHundredthsMm)
        {
            file.close();
            return false;
        }
    }
    file.close();
    return matches == 1U;
}

bool resolveImmutableSpoolAndBridge(fs::FS& storage, RunWireReference& reference)
{
    JobSpoolSelection selection;
    bool found = false;
    if (!JobSpoolSelectionStore::loadReadOnly(storage,
                                              reference.sourceSessionId,
                                              selection,
                                              found) ||
        !found || selection.repairId != reference.repairId ||
        selection.spoolId == 0UL ||
        selection.wireType != reference.materialClass ||
        selection.diameterHundredthsMm != reference.diameterHundredthsMm ||
        (reference.hasPersistedSpoolId &&
         reference.persistedSpoolId != selection.spoolId))
    {
        return false;
    }
    reference.spoolId = selection.spoolId;
    return bridgeMatches(storage, reference);
}

bool resolveLedgerBatch(fs::FS& storage, RunWireReference* references, uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(UsagePath)) return false;
    File file = storage.open(UsagePath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }
        String comment;
        if (!findString(line, "comment", comment)) continue;
        String transactionRef;
        bool tagged = false;
        if (!extractRunWireTag(comment, transactionRef, tagged))
        {
            file.close();
            return false;
        }
        if (!tagged) continue;

        for (uint8_t index = 0U; index < count; ++index)
        {
            RunWireReference& reference = references[index];
            if (transactionRef != reference.transactionRef) continue;
            uint32_t repairId = 0UL, materialId = 0UL, quantity = 0UL;
            uint32_t pricePerUnitMinor = 0UL;
            uint64_t lineCost = 0ULL;
            String currency, timestamp;
            if (++reference.ledgerMatches > 1U ||
                !findUnsigned(line, "repair_id", repairId) ||
                !findUnsigned(line, "material_id", materialId) ||
                !findUnsigned(line, "quantity_milli", quantity) ||
                !findUnsigned(line, "price_per_unit_minor", pricePerUnitMinor) ||
                !findUnsigned64(line, "line_cost_minor", lineCost) ||
                !findString(line, "currency", currency) ||
                !findString(line, "timestamp", timestamp) ||
                reference.unitCostMinor == 0ULL ||
                (reference.unitCostMinor % 1000ULL) != 0ULL ||
                reference.unitCostMinor / 1000ULL > 0xFFFFFFFFULL ||
                pricePerUnitMinor !=
                    static_cast<uint32_t>(reference.unitCostMinor / 1000ULL) ||
                repairId != reference.repairId ||
                materialId != reference.warehouseItemId ||
                quantity != reference.ledgerQuantityMilli ||
                lineCost != reference.costAmountMinor ||
                currency != reference.currency || timestamp != reference.createdAt)
            {
                file.close();
                return false;
            }
        }
    }
    file.close();

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (references[index].ledgerMatches != 1U) return false;
    }
    return true;
}

bool resolveWarehouseBatch(fs::FS& storage, RunWireReference* references, uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(WarehouseMovementsPath)) return false;
    File file = storage.open(WarehouseMovementsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

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
        if (record.status != "CONFIRMED") continue;
        String transactionRef;
        bool tagged = false;
        if (!extractRunWireTag(record.comment, transactionRef, tagged))
        {
            file.close();
            return false;
        }
        if (!tagged) continue;

        for (uint8_t index = 0U; index < count; ++index)
        {
            RunWireReference& reference = references[index];
            if (transactionRef != reference.transactionRef) continue;
            if (++reference.warehouseMatches > 1U ||
                reference.unitCostMinor == 0ULL ||
                reference.unitCostMinor > 0xFFFFFFFFULL ||
                record.pricePerKgMinor != static_cast<uint32_t>(reference.unitCostMinor) ||
                record.mode != WarehouseWriteOffMode::KgFirst ||
                record.stockMode != WarehouseWriteOffStockMode::Spool ||
                !record.hasSpoolId || record.spoolId != reference.spoolId ||
                record.repairId != reference.repairId ||
                !record.hasSourceSessionId ||
                record.sourceSessionId != reference.sourceSessionId ||
                !record.hasSourceRunId || record.sourceRunId != reference.sourceRunId ||
                !record.hasWireType || record.wireType != reference.materialClass ||
                record.diameterHundredthsMm != reference.diameterHundredthsMm ||
                record.massGrams != reference.consumedGrams ||
                record.currency != reference.currency || record.timestamp != reference.createdAt)
            {
                file.close();
                return false;
            }
        }
    }
    file.close();

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (references[index].warehouseMatches != 1U) return false;
    }
    return true;
}

bool resolveBatch(fs::FS& storage, RunWireReference* references, uint8_t count)
{
    for (uint8_t index = 0U; index < count; ++index)
    {
        references[index].ledgerMatches = 0U;
        references[index].warehouseMatches = 0U;
        if (!resolveImmutableSpoolAndBridge(storage, references[index])) return false;
    }
    return resolveLedgerBatch(storage, references, count) &&
           resolveWarehouseBatch(storage, references, count);
}

bool countTaggedLedgerEvidence(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(UsagePath)) return true;
    File file = storage.open(UsagePath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }
        String comment;
        if (!findString(line, "comment", comment)) continue;
        String transactionRef;
        bool tagged = false;
        if (!extractRunWireTag(comment, transactionRef, tagged))
        {
            file.close();
            return false;
        }
        if (!tagged) continue;
        if (count == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++count;
    }
    file.close();
    return true;
}

bool countTaggedWarehouseEvidence(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(WarehouseMovementsPath)) return true;
    File file = storage.open(WarehouseMovementsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
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
        if (record.status != "CONFIRMED") continue;
        String transactionRef;
        bool tagged = false;
        if (!extractRunWireTag(record.comment, transactionRef, tagged))
        {
            file.close();
            return false;
        }
        if (!tagged) continue;
        if (count == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++count;
    }
    file.close();
    return true;
}

bool checkMovements(fs::FS& storage, uint32_t& movementCount)
{
    movementCount = 0UL;
    if (!storage.exists(MaterialRequestMovementStore::Path)) return true;
    File file = storage.open(MaterialRequestMovementStore::Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    RunWireReference references[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    uint32_t previousMovementId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t movementId = 0UL;
        String sourceKind;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            movementId <= previousMovementId ||
            !findString(line, "source_kind", sourceKind))
        {
            file.close();
            return false;
        }
        previousMovementId = movementId;
        if (sourceKind != "RUN_WIRE") continue;

        RunWireReference& reference = references[batchCount];
        reference = RunWireReference();
        uint32_t diameter = 0UL;
        String movementKind, unit;
        if (!findString(line, "transaction_ref", reference.transactionRef) ||
            reference.transactionRef.length() < 8U ||
            reference.transactionRef.length() > 80U ||
            reference.transactionRef.indexOf(F("RWI-")) != 0 ||
            !findUnsigned(line, "repair_id", reference.repairId) ||
            reference.repairId == 0UL ||
            !findUnsigned(line, "warehouse_item_id", reference.warehouseItemId) ||
            reference.warehouseItemId == 0UL ||
            !findString(line, "movement_kind", movementKind) || movementKind != "ISSUE" ||
            !findUnsigned(line, "quantity_milli_units", reference.consumedGrams) ||
            reference.consumedGrams == 0UL ||
            reference.consumedGrams > 0xFFFFFFFFUL / 1000UL ||
            !findString(line, "unit", unit) || unit != "KG" ||
            !findUnsigned64(line, "unit_cost_minor", reference.unitCostMinor) ||
            reference.unitCostMinor == 0ULL ||
            reference.unitCostMinor > 0xFFFFFFFFULL ||
            !findUnsigned64(line, "cost_amount_minor", reference.costAmountMinor) ||
            !findString(line, "currency", reference.currency) ||
            reference.currency != "KGS" ||
            !findString(line, "created_at", reference.createdAt) ||
            reference.createdAt.length() < 10U ||
            !findUnsigned(line, "source_session_id", reference.sourceSessionId) ||
            reference.sourceSessionId == 0UL ||
            !findUnsigned(line, "source_run_id", reference.sourceRunId) ||
            reference.sourceRunId == 0UL ||
            !findString(line, "material_class", reference.materialClass) ||
            (reference.materialClass != "CU" && reference.materialClass != "AL") ||
            !findUnsigned(line, "wire_diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 500UL)
        {
            file.close();
            return false;
        }

        const String spoolMarker = F("\"spool_id\":");
        const int spoolPosition = line.indexOf(spoolMarker);
        if (spoolPosition >= 0)
        {
            if (!findUnsigned(line, "spool_id", reference.persistedSpoolId) ||
                reference.persistedSpoolId == 0UL)
            {
                file.close();
                return false;
            }
            reference.hasPersistedSpoolId = true;
        }

        reference.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        reference.ledgerQuantityMilli = reference.consumedGrams * 1000UL;

        if (movementCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++movementCount;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!resolveBatch(storage, references, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U && !resolveBatch(storage, references, batchCount))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}
}

bool RunWireAccountingIntegrityAudit::check(fs::FS& storage)
{
    RunWireAccountingIntegrityMetrics ignoredMetrics;
    return check(storage, ignoredMetrics);
}

bool RunWireAccountingIntegrityAudit::check(
    fs::FS& storage,
    RunWireAccountingIntegrityMetrics& metrics)
{
    metrics = RunWireAccountingIntegrityMetrics();

    // Cross-log integrity must never try to interpret an in-flight transaction.
    // RunWireIssueCoordinator owns deterministic recovery of these files.
    if (storage.exists(RunWireIssuePendingStore::Path) ||
        storage.exists(RunWireIssuePendingStore::TempPath))
    {
        return false;
    }

    if (!checkMovements(storage, metrics.movementCount) ||
        !countTaggedLedgerEvidence(storage, metrics.ledgerEvidenceCount) ||
        !countTaggedWarehouseEvidence(storage, metrics.warehouseEvidenceCount))
    {
        return false;
    }

    // Every completed RUN_WIRE transaction contributes exactly one immutable
    // request movement, one MaterialLedger usage and one warehouse CONFIRMED
    // write-off. Equality also rejects orphan system-owned RWI_TX evidence.
    return metrics.movementCount == metrics.ledgerEvidenceCount &&
           metrics.movementCount == metrics.warehouseEvidenceCount;
}
}
