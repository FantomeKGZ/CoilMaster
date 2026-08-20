#include "CM_WarehouseMovementIntegrityAudit.h"
#include "CM_WarehouseWriteOffRecord.h"
#include <Arduino.h>

namespace CM
{
namespace
{
bool sameTransactionCore(const WarehouseWriteOffRecord& pending,
                         const WarehouseWriteOffRecord& finalRecord)
{
    if (pending.movementId != finalRecord.movementId ||
        pending.mode != finalRecord.mode ||
        pending.stockMode != finalRecord.stockMode ||
        pending.hasSpoolId != finalRecord.hasSpoolId ||
        pending.spoolId != finalRecord.spoolId ||
        pending.repairId != finalRecord.repairId ||
        pending.hasSourceSessionId != finalRecord.hasSourceSessionId ||
        pending.sourceSessionId != finalRecord.sourceSessionId ||
        pending.hasSourceRunId != finalRecord.hasSourceRunId ||
        pending.sourceRunId != finalRecord.sourceRunId ||
        pending.hasWeights != finalRecord.hasWeights ||
        pending.weightBeforeGrams != finalRecord.weightBeforeGrams ||
        pending.weightAfterGrams != finalRecord.weightAfterGrams ||
        pending.massGrams != finalRecord.massGrams ||
        pending.quantityKg != finalRecord.quantityKg ||
        pending.pricePerKgMinor != finalRecord.pricePerKgMinor ||
        pending.currency != finalRecord.currency ||
        pending.timestamp != finalRecord.timestamp ||
        pending.comment != finalRecord.comment)
    {
        return false;
    }

    // Legacy PENDING/ABORTED deliberately have no conductor snapshot; the
    // durable spool mutation supplies it only on CONFIRMED. KG_FIRST records
    // carry an immutable conductor snapshot through the whole transaction.
    if (pending.mode == WarehouseWriteOffMode::KgFirst)
    {
        return pending.diameterHundredthsMm == finalRecord.diameterHundredthsMm &&
               pending.hasWireType == finalRecord.hasWireType &&
               pending.wireType == finalRecord.wireType;
    }
    return true;
}

bool confirmedProvenanceUnique(fs::FS& storage, const char* path)
{
    File outer = storage.open(path, FILE_READ);
    if (!outer || outer.isDirectory())
    {
        if (outer) outer.close();
        return false;
    }

    while (outer.available())
    {
        const String line = outer.readStringUntil('\n');
        if (line.length() == 0U) continue;
        WarehouseWriteOffRecord record;
        if (!WarehouseWriteOffRecordCodec::parse(line, record))
        {
            outer.close();
            return false;
        }
        if (record.status != "CONFIRMED" || !record.hasSourceSessionId) continue;

        File inner = storage.open(path, FILE_READ);
        if (!inner || inner.isDirectory())
        {
            if (inner) inner.close();
            outer.close();
            return false;
        }

        while (inner.available())
        {
            const String candidateLine = inner.readStringUntil('\n');
            if (candidateLine.length() == 0U) continue;
            WarehouseWriteOffRecord candidate;
            if (!WarehouseWriteOffRecordCodec::parse(candidateLine, candidate))
            {
                inner.close();
                outer.close();
                return false;
            }
            if (candidate.status != "CONFIRMED" ||
                candidate.movementId == record.movementId ||
                !candidate.hasSourceSessionId ||
                candidate.sourceSessionId != record.sourceSessionId)
            {
                continue;
            }

            // A legacy session-level write-off conflicts with every other
            // confirmed write-off for that session. Run-level records conflict
            // only when the exact run is duplicated, regardless of mode.
            if (!record.hasSourceRunId || !candidate.hasSourceRunId ||
                candidate.sourceRunId == record.sourceRunId)
            {
                inner.close();
                outer.close();
                return false;
            }
        }
        inner.close();
    }
    outer.close();
    return true;
}
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage)
{
    uint32_t ignoredRecordCount = 0UL;
    return check(storage, ignoredRecordCount);
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage,
                                            uint32_t& validatedRecordCount)
{
    validatedRecordCount = 0UL;
    constexpr const char* Path = "/data/warehouse/movements.ndjson";
    if (!storage.exists(Path)) return true;

    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t recordCount = 0UL;
    uint32_t maximumId = 0UL;
    bool hasPending = false;
    WarehouseWriteOffRecord pending;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        WarehouseWriteOffRecord record;
        if (!WarehouseWriteOffRecordCodec::parse(line, record) ||
            recordCount == 0xFFFFFFFFUL)
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
            ++recordCount;
            continue;
        }

        if (!hasPending || !sameTransactionCore(pending, record))
        {
            file.close();
            return false;
        }

        hasPending = false;
        pending = WarehouseWriteOffRecord();
        ++recordCount;
    }
    file.close();

    if (hasPending) return false;

    // Syntax and transaction shape were already validated exactly once above;
    // the second pass enforces source-session/run uniqueness across legacy and
    // KG_FIRST records without weakening old exact-run provenance.
    if (!confirmedProvenanceUnique(storage, Path)) return false;

    validatedRecordCount = recordCount;
    return true;
}
}
