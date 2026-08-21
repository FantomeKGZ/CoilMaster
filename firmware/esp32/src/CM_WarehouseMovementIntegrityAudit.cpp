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

    if (pending.mode == WarehouseWriteOffMode::KgFirst)
    {
        return pending.diameterHundredthsMm == finalRecord.diameterHundredthsMm &&
               pending.hasWireType == finalRecord.hasWireType &&
               pending.wireType == finalRecord.wireType;
    }
    return true;
}

bool addChecked64(uint64_t& target, uint64_t value)
{
    if (target > 0xFFFFFFFFFFFFFFFFULL - value) return false;
    target += value;
    return true;
}

bool addChecked32(uint32_t& target, uint32_t value)
{
    if (target > 0xFFFFFFFFUL - value) return false;
    target += value;
    return true;
}

bool accumulateRepairRecord(const WarehouseWriteOffRecord& record,
                            uint32_t repairId,
                            WarehouseMovementRepairTotals& totals)
{
    if (record.status != "CONFIRMED" || record.repairId != repairId) return true;

    const uint64_t product = static_cast<uint64_t>(record.massGrams) *
                             static_cast<uint64_t>(record.pricePerKgMinor);
    if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL) return false;
    const uint64_t lineCost = (product + 500ULL) / 1000ULL;

    if (totals.wireLineCount == 0xFFFFU ||
        !addChecked64(totals.wireCostMinor, lineCost))
        return false;
    ++totals.wireLineCount;

    if (!totals.currencySet)
    {
        totals.currency = record.currency;
        totals.currencySet = true;
    }
    else if (totals.currency != record.currency)
    {
        return false;
    }

    if (record.wireType == "CU")
    {
        if (totals.copperWireLineCount == 0xFFFFU ||
            !addChecked64(totals.copperWireCostMinor, lineCost) ||
            !addChecked32(totals.copperWireGrams, record.massGrams))
            return false;
        ++totals.copperWireLineCount;
        return true;
    }
    if (record.wireType == "AL")
    {
        if (totals.aluminiumWireLineCount == 0xFFFFU ||
            !addChecked64(totals.aluminiumWireCostMinor, lineCost) ||
            !addChecked32(totals.aluminiumWireGrams, record.massGrams))
            return false;
        ++totals.aluminiumWireLineCount;
        return true;
    }

    if (totals.unknownWireLineCount == 0xFFFFU ||
        !addChecked64(totals.unknownWireCostMinor, lineCost) ||
        !addChecked32(totals.unknownWireGrams, record.massGrams))
        return false;
    ++totals.unknownWireLineCount;
    return true;
}

struct ProvenanceEntry
{
    uint32_t movementId;
    uint32_t sourceSessionId;
    bool hasSourceRunId;
    uint32_t sourceRunId;
};

bool provenanceConflicts(const ProvenanceEntry& entry,
                         const WarehouseWriteOffRecord& candidate)
{
    if (candidate.status != "CONFIRMED" ||
        candidate.movementId == entry.movementId ||
        !candidate.hasSourceSessionId ||
        candidate.sourceSessionId != entry.sourceSessionId)
    {
        return false;
    }

    return !entry.hasSourceRunId || !candidate.hasSourceRunId ||
           candidate.sourceRunId == entry.sourceRunId;
}

bool confirmedProvenanceUnique(fs::FS& storage, const char* path)
{
    constexpr uint8_t BatchSize = 32U;
    ProvenanceEntry batch[BatchSize];

    File outer = storage.open(path, FILE_READ);
    if (!outer || outer.isDirectory())
    {
        if (outer) outer.close();
        return false;
    }

    while (outer.available())
    {
        uint8_t batchCount = 0U;
        while (outer.available() && batchCount < BatchSize)
        {
            const String line = outer.readStringUntil('\n');
            if (line.length() == 0U) continue;

            WarehouseWriteOffRecord record;
            if (!WarehouseWriteOffRecordCodec::parse(line, record))
            {
                outer.close();
                return false;
            }
            if (record.status != "CONFIRMED" || !record.hasSourceSessionId)
                continue;

            ProvenanceEntry& entry = batch[batchCount++];
            entry.movementId = record.movementId;
            entry.sourceSessionId = record.sourceSessionId;
            entry.hasSourceRunId = record.hasSourceRunId;
            entry.sourceRunId = record.sourceRunId;
        }

        if (batchCount == 0U) continue;

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

            for (uint8_t index = 0U; index < batchCount; ++index)
            {
                if (provenanceConflicts(batch[index], candidate))
                {
                    inner.close();
                    outer.close();
                    return false;
                }
            }
        }
        inner.close();
    }

    outer.close();
    return true;
}

bool checkInternal(fs::FS& storage,
                   uint32_t& validatedRecordCount,
                   uint32_t repairId,
                   WarehouseMovementRepairTotals* totals)
{
    validatedRecordCount = 0UL;
    if (totals != nullptr) *totals = WarehouseMovementRepairTotals();

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

        if (totals != nullptr && !accumulateRepairRecord(record, repairId, *totals))
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
    if (!confirmedProvenanceUnique(storage, Path)) return false;

    validatedRecordCount = recordCount;
    return true;
}
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage)
{
    uint32_t ignoredRecordCount = 0UL;
    return checkInternal(storage, ignoredRecordCount, 0UL, nullptr);
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage,
                                            uint32_t& validatedRecordCount)
{
    return checkInternal(storage, validatedRecordCount, 0UL, nullptr);
}

bool WarehouseMovementIntegrityAudit::checkRepair(
    fs::FS& storage,
    uint32_t repairId,
    WarehouseMovementRepairTotals& totals)
{
    if (repairId == 0UL) return false;
    uint32_t ignoredRecordCount = 0UL;
    return checkInternal(storage, ignoredRecordCount, repairId, &totals);
}
}
