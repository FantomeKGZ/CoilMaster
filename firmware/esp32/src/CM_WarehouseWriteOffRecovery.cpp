#include "CM_WarehouseStore.h"
#include "CM_WarehouseWriteOffRecord.h"

namespace CM
{
namespace
{
bool sameTransactionCore(const WarehouseWriteOffRecord& left,
                         const WarehouseWriteOffRecord& right)
{
    if (left.mode != right.mode || left.stockMode != right.stockMode ||
        left.movementId != right.movementId ||
        left.hasSpoolId != right.hasSpoolId || left.spoolId != right.spoolId ||
        left.repairId != right.repairId ||
        left.hasSourceSessionId != right.hasSourceSessionId ||
        left.sourceSessionId != right.sourceSessionId ||
        left.hasSourceRunId != right.hasSourceRunId ||
        left.sourceRunId != right.sourceRunId ||
        left.hasWeights != right.hasWeights ||
        left.weightBeforeGrams != right.weightBeforeGrams ||
        left.weightAfterGrams != right.weightAfterGrams ||
        left.massGrams != right.massGrams ||
        left.pricePerKgMinor != right.pricePerKgMinor ||
        left.currency != right.currency || left.timestamp != right.timestamp ||
        left.comment != right.comment)
    {
        return false;
    }

    if (left.mode == WarehouseWriteOffMode::KgFirst)
    {
        return left.diameterHundredthsMm == right.diameterHundredthsMm &&
               left.hasWireType == right.hasWireType &&
               left.wireType == right.wireType &&
               left.quantityKg == right.quantityKg;
    }
    return true;
}
}

bool WarehouseStore::recoverPendingWriteOff()
{
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximumId = 0UL;
    bool hasPending = false;
    WarehouseWriteOffRecord pending;

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

        if (record.status == "PENDING")
        {
            if (hasPending || record.movementId <= maximumId)
            {
                file.close();
                return false;
            }
            maximumId = record.movementId;
            pending = record;
            hasPending = true;
            continue;
        }

        if (!hasPending || record.movementId != pending.movementId ||
            !sameTransactionCore(pending, record))
        {
            file.close();
            return false;
        }
        hasPending = false;
        pending = WarehouseWriteOffRecord();
    }
    file.close();

    if (!hasPending) return true;

    WarehousePrice price;
    price.pricePerKgMinor = pending.pricePerKgMinor;
    price.currency = pending.currency;

    if (pending.mode == WarehouseWriteOffMode::KgFirst &&
        pending.stockMode == WarehouseWriteOffStockMode::Unallocated)
    {
        KgFirstWriteOff operation;
        operation.repairId = pending.repairId;
        operation.sourceSessionId = pending.sourceSessionId;
        operation.sourceRunId = pending.sourceRunId;
        operation.diameterHundredthsMm = pending.diameterHundredthsMm;
        operation.consumedGrams = pending.massGrams;
        operation.wireType = pending.wireType;
        operation.timestamp = pending.timestamp;
        operation.comment = pending.comment;
        // No stock mutation ever occurs for UNALLOCATED. A dangling PENDING can
        // therefore only be closed as ABORTED after reboot; it must never be
        // guessed into a confirmed material deduction.
        return appendKgFirstWriteOffRecord(pending.movementId,
                                           operation,
                                           0UL,
                                           0UL,
                                           price,
                                           "ABORTED");
    }

    File spools = m_storage.open(SpoolsPath, FILE_READ);
    if (!spools) return false;

    bool found = false;
    uint32_t previousSpoolId = 0UL;
    uint32_t currentWeight = 0UL;
    uint16_t diameter = 0U;
    String wireType;
    while (spools.available())
    {
        const String line = spools.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t spoolId = 0UL;
        uint32_t weight = 0UL;
        uint32_t lineDiameter = 0UL;
        String status, lineWireType;
        if (!findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousSpoolId ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findUnsigned(line, "diameter_hundredths_mm", lineDiameter) ||
            lineDiameter == 0UL || lineDiameter > 0xFFFFUL ||
            !findString(line, "status", status) ||
            !findString(line, "wire_type", lineWireType) ||
            (lineWireType != "CU" && lineWireType != "AL"))
        {
            spools.close();
            return false;
        }
        previousSpoolId = spoolId;

        if (!pending.hasSpoolId || spoolId != pending.spoolId) continue;
        if (found || status != "ACTIVE")
        {
            spools.close();
            return false;
        }
        found = true;
        currentWeight = weight;
        diameter = static_cast<uint16_t>(lineDiameter);
        wireType = lineWireType;
    }
    spools.close();
    if (!found) return false;

    if (pending.mode == WarehouseWriteOffMode::LegacySpool)
    {
        ConfirmedSpoolWriteOff operation;
        operation.spoolId = pending.spoolId;
        operation.repairId = pending.repairId;
        operation.sourceSessionId = pending.sourceSessionId;
        operation.sourceRunId = pending.sourceRunId;
        operation.weightBeforeGrams = pending.weightBeforeGrams;
        operation.weightAfterGrams = pending.weightAfterGrams;
        operation.timestamp = pending.timestamp;
        operation.comment = pending.comment;

        if (currentWeight == pending.weightBeforeGrams)
        {
            return appendWriteOffRecord(pending.movementId,
                                        operation,
                                        0U,
                                        pending.massGrams,
                                        price,
                                        "ABORTED",
                                        String());
        }

        if (currentWeight == pending.weightAfterGrams)
        {
            return appendWriteOffRecord(pending.movementId,
                                        operation,
                                        diameter,
                                        pending.massGrams,
                                        price,
                                        "CONFIRMED",
                                        wireType);
        }
        return false;
    }

    if (pending.stockMode != WarehouseWriteOffStockMode::Spool ||
        diameter != pending.diameterHundredthsMm || wireType != pending.wireType)
    {
        return false;
    }

    KgFirstWriteOff operation;
    operation.spoolId = pending.spoolId;
    operation.repairId = pending.repairId;
    operation.sourceSessionId = pending.sourceSessionId;
    operation.sourceRunId = pending.sourceRunId;
    operation.diameterHundredthsMm = pending.diameterHundredthsMm;
    operation.consumedGrams = pending.massGrams;
    operation.wireType = pending.wireType;
    operation.timestamp = pending.timestamp;
    operation.comment = pending.comment;

    if (currentWeight == pending.weightBeforeGrams)
    {
        return appendKgFirstWriteOffRecord(pending.movementId,
                                           operation,
                                           pending.weightBeforeGrams,
                                           pending.weightAfterGrams,
                                           price,
                                           "ABORTED");
    }
    if (currentWeight == pending.weightAfterGrams)
    {
        return appendKgFirstWriteOffRecord(pending.movementId,
                                           operation,
                                           pending.weightBeforeGrams,
                                           pending.weightAfterGrams,
                                           price,
                                           "CONFIRMED");
    }
    return false;
}
}
