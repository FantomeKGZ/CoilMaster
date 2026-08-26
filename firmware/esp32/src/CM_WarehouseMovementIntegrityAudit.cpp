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

WarehouseMovementDiameterTotals* findOrCreateSummaryDiameter(
    WarehouseMovementSummaryTotals& totals,
    uint16_t diameterHundredthsMm)
{
    if (diameterHundredthsMm == 0U) return nullptr;
    for (uint8_t index = 0U; index < totals.diameterCount; ++index)
    {
        if (totals.diameters[index].diameterHundredthsMm == diameterHundredthsMm)
            return &totals.diameters[index];
    }
    if (totals.diameterCount >= WarehouseMovementSummaryMaxDiameters)
        return nullptr;

    WarehouseMovementDiameterTotals& item = totals.diameters[totals.diameterCount++];
    item = WarehouseMovementDiameterTotals();
    item.diameterHundredthsMm = diameterHundredthsMm;
    return &item;
}

bool accumulateSummaryRecord(const WarehouseWriteOffRecord& record,
                             const char* monthPrefix,
                             WarehouseMovementSummaryTotals& totals)
{
    if (record.status != "CONFIRMED") return true;
    if (record.diameterHundredthsMm == 0U || record.massGrams == 0UL)
        return false;

    WarehouseMovementDiameterTotals* item =
        findOrCreateSummaryDiameter(totals, record.diameterHundredthsMm);
    if (item == nullptr ||
        !addChecked32(item->consumedAllTimeGrams, record.massGrams))
    {
        return false;
    }

    if (record.timestamp.startsWith(monthPrefix) &&
        !addChecked32(item->consumedMonthGrams, record.massGrams))
    {
        return false;
    }
    return true;
}

bool accumulateCoverageRecord(const WarehouseWriteOffRecord& record,
                              uint32_t repairId,
                              WarehouseMovementCoverageTarget* targets,
                              uint8_t targetCount)
{
    if (record.status != "CONFIRMED" || !record.hasSourceSessionId) return true;

    for (uint8_t index = 0U; index < targetCount; ++index)
    {
        WarehouseMovementCoverageTarget& target = targets[index];
        if (record.sourceSessionId != target.sessionId) continue;
        if (record.repairId != repairId) return false;

        bool matches = false;
        if (record.mode == WarehouseWriteOffMode::LegacySpool)
        {
            if (!record.hasSpoolId || record.spoolId != target.spoolId) return false;
            if (!record.hasSourceRunId) return false;
            matches = record.sourceRunId == target.runId;
        }
        else
        {
            if (!record.hasSourceRunId || record.sourceRunId != target.runId)
                continue;
            if (record.stockMode == WarehouseWriteOffStockMode::Spool)
            {
                if (!record.hasSpoolId || record.spoolId != target.spoolId) return false;
            }
            else if (record.stockMode == WarehouseWriteOffStockMode::Unallocated)
            {
                if (record.hasSpoolId) return false;
            }
            else
            {
                return false;
            }
            matches = true;
        }

        if (!matches) continue;
        if (target.confirmed) return false;
        target.confirmed = true;
    }
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

bool checkInternalWithSummary(fs::FS& storage,
                              uint32_t& validatedRecordCount,
                              uint32_t repairId,
                              WarehouseMovementRepairTotals* totals,
                              const char* monthPrefix,
                              WarehouseMovementSummaryTotals* summaryTotals,
                              uint32_t sourceSessionId,
                              uint32_t sourceRunId,
                              bool* sourceRunConfirmed,
                              WarehouseMovementCoverageTarget* coverageTargets,
                              uint8_t coverageTargetCount)
{
    validatedRecordCount = 0UL;
    if (totals != nullptr) *totals = WarehouseMovementRepairTotals();
    if (summaryTotals != nullptr) *summaryTotals = WarehouseMovementSummaryTotals();
    if (sourceRunConfirmed != nullptr) *sourceRunConfirmed = false;
    if (coverageTargetCount > WarehouseMovementCoverageMaxTargets ||
        (coverageTargetCount > 0U && coverageTargets == nullptr))
    {
        return false;
    }
    for (uint8_t index = 0U; index < coverageTargetCount; ++index)
    {
        if (coverageTargets[index].sessionId == 0UL ||
            coverageTargets[index].runId == 0UL ||
            coverageTargets[index].spoolId == 0UL)
        {
            return false;
        }
        coverageTargets[index].confirmed = false;
    }
    if (summaryTotals != nullptr &&
        (monthPrefix == nullptr || String(monthPrefix).length() != 7U))
    {
        return false;
    }

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
        if (summaryTotals != nullptr &&
            !accumulateSummaryRecord(record, monthPrefix, *summaryTotals))
        {
            file.close();
            return false;
        }
        if (coverageTargets != nullptr &&
            !accumulateCoverageRecord(record, repairId,
                                      coverageTargets, coverageTargetCount))
        {
            file.close();
            return false;
        }

        if (sourceRunConfirmed != nullptr && record.status == "CONFIRMED" &&
            record.hasSourceSessionId && record.hasSourceRunId &&
            record.sourceSessionId == sourceSessionId &&
            record.sourceRunId == sourceRunId)
        {
            *sourceRunConfirmed = true;
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

bool checkInternal(fs::FS& storage,
                   uint32_t& validatedRecordCount,
                   uint32_t repairId,
                   WarehouseMovementRepairTotals* totals,
                   uint32_t sourceSessionId,
                   uint32_t sourceRunId,
                   bool* sourceRunConfirmed)
{
    return checkInternalWithSummary(storage, validatedRecordCount, repairId, totals,
                                    nullptr, nullptr, sourceSessionId, sourceRunId,
                                    sourceRunConfirmed, nullptr, 0U);
}
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage)
{
    uint32_t ignoredRecordCount = 0UL;
    return checkInternal(storage, ignoredRecordCount, 0UL, nullptr, 0UL, 0UL, nullptr);
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage,
                                            uint32_t& validatedRecordCount)
{
    return checkInternal(storage, validatedRecordCount, 0UL, nullptr, 0UL, 0UL, nullptr);
}

bool WarehouseMovementIntegrityAudit::checkSummary(
    fs::FS& storage,
    const char* monthPrefix,
    WarehouseMovementSummaryTotals& totals)
{
    uint32_t ignoredRecordCount = 0UL;
    return checkInternalWithSummary(storage, ignoredRecordCount, 0UL, nullptr,
                                    monthPrefix, &totals, 0UL, 0UL, nullptr,
                                    nullptr, 0U);
}

bool WarehouseMovementIntegrityAudit::checkRepair(
    fs::FS& storage,
    uint32_t repairId,
    WarehouseMovementRepairTotals& totals)
{
    if (repairId == 0UL) return false;
    uint32_t ignoredRecordCount = 0UL;
    return checkInternal(storage, ignoredRecordCount, repairId, &totals, 0UL, 0UL, nullptr);
}

bool WarehouseMovementIntegrityAudit::checkSourceRun(fs::FS& storage,
                                                      uint32_t sourceSessionId,
                                                      uint32_t sourceRunId,
                                                      bool& confirmed)
{
    confirmed = false;
    if (sourceSessionId == 0UL || sourceRunId == 0UL) return false;
    uint32_t ignoredRecordCount = 0UL;
    return checkInternal(storage, ignoredRecordCount, 0UL, nullptr,
                         sourceSessionId, sourceRunId, &confirmed);
}

bool WarehouseMovementIntegrityAudit::checkCoverageBatch(
    fs::FS& storage,
    uint32_t repairId,
    WarehouseMovementCoverageTarget* targets,
    uint8_t targetCount)
{
    if (repairId == 0UL || targetCount > WarehouseMovementCoverageMaxTargets ||
        (targetCount > 0U && targets == nullptr))
    {
        return false;
    }
    uint32_t ignoredRecordCount = 0UL;
    return checkInternalWithSummary(storage, ignoredRecordCount, repairId, nullptr,
                                    nullptr, nullptr, 0UL, 0UL, nullptr,
                                    targets, targetCount);
}
}
