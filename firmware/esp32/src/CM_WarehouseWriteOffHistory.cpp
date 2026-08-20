#include "CM_WarehouseStore.h"
#include "CM_WarehouseWriteOffRecord.h"

namespace CM
{
namespace
{
bool matchingTransaction(const WarehouseWriteOffRecord& pending,
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
        return false;

    if (pending.mode == WarehouseWriteOffMode::KgFirst)
    {
        return pending.diameterHundredthsMm == finalRecord.diameterHundredthsMm &&
               pending.hasWireType == finalRecord.hasWireType &&
               pending.wireType == finalRecord.wireType;
    }
    return true;
}
}

bool WarehouseStore::appendConfirmedWriteOffsPageJson(String& json,
                                                       uint32_t repairId,
                                                       uint32_t cursor,
                                                       uint8_t limit,
                                                       uint16_t& appendedCount,
                                                       uint16_t& totalMatchingCount,
                                                       uint32_t& nextCursor,
                                                       bool& hasMore,
                                                       uint32_t& totalConsumedGrams,
                                                       uint64_t& totalValueMinor,
                                                       WriteOffMaterialTotals& materialTotals) const
{
    appendedCount = 0U;
    totalMatchingCount = 0U;
    nextCursor = 0UL;
    hasMore = false;
    totalConsumedGrams = 0UL;
    totalValueMinor = 0ULL;
    materialTotals = WriteOffMaterialTotals();
    if (!ready() || repairId == 0UL || limit == 0U || limit > WarehouseMaxListPageSize)
        return false;
    if (!m_storage.exists(MovementsPath)) return cursor == 0UL;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;
    if (cursor > static_cast<uint32_t>(file.size()))
    {
        file.close();
        return false;
    }

    bool first = true;
    bool cursorSeen = cursor == 0UL;
    uint32_t pageEndCursor = 0UL;
    uint32_t maximumId = 0UL;
    bool hasPending = false;
    uint32_t pendingRecordStart = 0UL;
    WarehouseWriteOffRecord pending;

    while (file.available())
    {
        const uint32_t lineStart = static_cast<uint32_t>(file.position());
        if (lineStart == cursor) cursorSeen = true;
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
            pendingRecordStart = lineStart;
            hasPending = true;
            continue;
        }

        if (!hasPending || !matchingTransaction(pending, record))
        {
            file.close();
            return false;
        }

        if (record.status == "ABORTED")
        {
            hasPending = false;
            pending = WarehouseWriteOffRecord();
            continue;
        }

        if (record.repairId == repairId)
        {
            const uint64_t consumedValueMinor =
                (static_cast<uint64_t>(record.massGrams) *
                 static_cast<uint64_t>(record.pricePerKgMinor) + 500ULL) / 1000ULL;

            if (totalConsumedGrams > 0xFFFFFFFFUL - record.massGrams ||
                totalValueMinor > 0xFFFFFFFFFFFFFFFFULL - consumedValueMinor ||
                totalMatchingCount == 0xFFFFU)
            {
                file.close();
                return false;
            }

            uint32_t* gramsTarget = &materialTotals.unknownGrams;
            uint64_t* valueTarget = &materialTotals.unknownValueMinor;
            uint16_t* countTarget = &materialTotals.unknownCount;
            if (record.wireType == "CU")
            {
                gramsTarget = &materialTotals.copperGrams;
                valueTarget = &materialTotals.copperValueMinor;
                countTarget = &materialTotals.copperCount;
            }
            else if (record.wireType == "AL")
            {
                gramsTarget = &materialTotals.aluminiumGrams;
                valueTarget = &materialTotals.aluminiumValueMinor;
                countTarget = &materialTotals.aluminiumCount;
            }

            if (*gramsTarget > 0xFFFFFFFFUL - record.massGrams ||
                *valueTarget > 0xFFFFFFFFFFFFFFFFULL - consumedValueMinor ||
                *countTarget == 0xFFFFU)
            {
                file.close();
                return false;
            }

            ++totalMatchingCount;
            totalConsumedGrams += record.massGrams;
            totalValueMinor += consumedValueMinor;
            *gramsTarget += record.massGrams;
            *valueTarget += consumedValueMinor;
            ++(*countTarget);

            if (cursorSeen && pendingRecordStart >= cursor)
            {
                if (appendedCount >= limit)
                {
                    hasMore = true;
                }
                else
                {
                    char valueBuffer[24];
                    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
                             static_cast<unsigned long long>(consumedValueMinor));

                    if (!first) json += ',';
                    first = false;
                    json += F("{\"movement_id\":"); json += record.movementId;
                    json += F(",\"writeoff_mode\":\"");
                    json += record.mode == WarehouseWriteOffMode::KgFirst
                                ? F("KG_FIRST") : F("LEGACY_SPOOL");
                    json += F("\",\"stock_mode\":\"");
                    if (record.stockMode == WarehouseWriteOffStockMode::Unallocated)
                        json += F("UNALLOCATED");
                    else if (record.stockMode == WarehouseWriteOffStockMode::Spool)
                        json += F("SPOOL");
                    else
                        json += F("LEGACY_SPOOL");
                    json += F("\",\"spool_id\":");
                    if (record.hasSpoolId) json += record.spoolId;
                    else json += F("null");
                    json += F(",\"repair_id\":"); json += record.repairId;
                    json += F(",\"source_session_id\":");
                    if (record.hasSourceSessionId) json += record.sourceSessionId;
                    else json += F("null");
                    json += F(",\"source_run_id\":");
                    if (record.hasSourceRunId) json += record.sourceRunId;
                    else json += F("null");
                    json += F(",\"diameter_hundredths_mm\":");
                    json += record.diameterHundredthsMm;
                    json += F(",\"wire_type\":");
                    if (record.hasWireType)
                    {
                        json += '"'; json += jsonEscape(record.wireType); json += '"';
                    }
                    else json += F("null");
                    json += F(",\"legacy_unknown_material\":");
                    json += record.hasWireType ? F("false") : F("true");
                    json += F(",\"quantity_kg\":");
                    if (record.mode == WarehouseWriteOffMode::KgFirst)
                    {
                        json += '"'; json += jsonEscape(record.quantityKg); json += '"';
                    }
                    else json += F("null");
                    json += F(",\"weight_before_g\":");
                    if (record.hasWeights) json += record.weightBeforeGrams;
                    else json += F("null");
                    json += F(",\"weight_after_g\":");
                    if (record.hasWeights) json += record.weightAfterGrams;
                    else json += F("null");
                    json += F(",\"consumed_g\":"); json += record.massGrams;
                    json += F(",\"price_per_kg_minor\":"); json += record.pricePerKgMinor;
                    json += F(",\"consumed_value_minor\":"); json += valueBuffer;
                    json += F(",\"currency\":\""); json += jsonEscape(record.currency); json += '"';
                    json += F(",\"timestamp\":\""); json += jsonEscape(record.timestamp); json += '"';
                    if (record.comment.length() > 0U)
                    {
                        json += F(",\"comment\":\"");
                        json += jsonEscape(record.comment); json += '"';
                    }
                    json += '}';

                    ++appendedCount;
                    pageEndCursor = static_cast<uint32_t>(file.position());
                }
            }
        }

        hasPending = false;
        pending = WarehouseWriteOffRecord();
    }

    const uint32_t endPosition = static_cast<uint32_t>(file.position());
    file.close();
    if (cursor == endPosition) cursorSeen = true;
    if (hasMore) nextCursor = pageEndCursor;
    // WarehouseStore::begin() reconciles a dangling PENDING before m_ready=true.
    // Seeing one here therefore means runtime corruption or storage loss.
    return !hasPending && cursorSeen;
}
}
