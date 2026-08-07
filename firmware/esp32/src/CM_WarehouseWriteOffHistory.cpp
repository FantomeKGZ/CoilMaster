#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::appendConfirmedWriteOffsJson(String& json,
                                                   uint32_t repairId,
                                                   uint16_t& appendedCount,
                                                   uint32_t& totalConsumedGrams,
                                                   uint64_t& totalValueMinor,
                                                   WriteOffMaterialTotals& materialTotals) const
{
    appendedCount = 0U;
    totalConsumedGrams = 0UL;
    totalValueMinor = 0ULL;
    materialTotals = WriteOffMaterialTotals();
    if (!m_ready || repairId == 0UL) return false;
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    uint32_t maximumId = 0UL;
    uint32_t pendingId = 0UL;
    uint32_t pendingSpoolId = 0UL;
    uint32_t pendingRepairId = 0UL;
    uint32_t pendingBefore = 0UL;
    uint32_t pendingAfter = 0UL;
    uint32_t pendingMass = 0UL;
    uint32_t pendingPrice = 0UL;
    String pendingCurrency;
    String pendingTimestamp;
    String pendingComment;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentRepairId = 0UL;
        uint32_t movementId = 0UL;
        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t before = 0UL;
        uint32_t after = 0UL;
        uint32_t mass = 0UL;
        uint32_t price = 0UL;
        String type, status, wireType, currency, timestamp, comment;

        if (!findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status) ||
            !findUnsigned(line, "repair_id", currentRepairId) || currentRepairId == 0UL ||
            !findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) || diameter > 0xFFFFUL ||
            !findUnsigned(line, "weight_before_g", before) || before == 0UL ||
            !findUnsigned(line, "weight_after_g", after) || after >= before ||
            !findUnsigned(line, "mass_g", mass) || mass != before - after ||
            !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
        {
            file.close();
            return false;
        }

        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (hasWireType && (!findString(line, "wire_type", wireType) ||
                            (wireType != "CU" && wireType != "AL")))
        {
            file.close();
            return false;
        }

        const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
        if (hasComment && !findString(line, "comment", comment))
        {
            file.close();
            return false;
        }

        if (status == "PENDING")
        {
            if (pendingId != 0UL || movementId <= maximumId ||
                diameter != 0UL || hasWireType)
            {
                file.close();
                return false;
            }

            maximumId = movementId;
            pendingId = movementId;
            pendingSpoolId = spoolId;
            pendingRepairId = currentRepairId;
            pendingBefore = before;
            pendingAfter = after;
            pendingMass = mass;
            pendingPrice = price;
            pendingCurrency = currency;
            pendingTimestamp = timestamp;
            pendingComment = comment;
            continue;
        }

        if (status != "CONFIRMED" && status != "ABORTED")
        {
            file.close();
            return false;
        }

        if (pendingId == 0UL || movementId != pendingId ||
            spoolId != pendingSpoolId || currentRepairId != pendingRepairId ||
            before != pendingBefore || after != pendingAfter || mass != pendingMass ||
            price != pendingPrice || currency != pendingCurrency ||
            timestamp != pendingTimestamp || comment != pendingComment)
        {
            file.close();
            return false;
        }

        if (status == "ABORTED")
        {
            if (diameter != 0UL || hasWireType)
            {
                file.close();
                return false;
            }
            pendingId = 0UL;
            continue;
        }

        if (diameter == 0UL)
        {
            file.close();
            return false;
        }

        if (currentRepairId == repairId)
        {
            const uint64_t consumedValueMinor =
                (static_cast<uint64_t>(mass) * static_cast<uint64_t>(price) + 500ULL) / 1000ULL;

            if (totalConsumedGrams > 0xFFFFFFFFUL - mass ||
                totalValueMinor > 0xFFFFFFFFFFFFFFFFULL - consumedValueMinor ||
                appendedCount == 0xFFFFU)
            {
                file.close();
                return false;
            }

            char valueBuffer[24];
            snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
                     static_cast<unsigned long long>(consumedValueMinor));

            if (!first) json += ',';
            first = false;
            json += F("{\"movement_id\":"); json += movementId;
            json += F(",\"spool_id\":"); json += spoolId;
            json += F(",\"repair_id\":"); json += currentRepairId;
            json += F(",\"diameter_hundredths_mm\":"); json += diameter;
            json += F(",\"wire_type\":");
            if (hasWireType)
            {
                json += '"'; json += jsonEscape(wireType); json += '"';
            }
            else json += F("null");
            json += F(",\"legacy_unknown_material\":");
            json += hasWireType ? F("false") : F("true");
            json += F(",\"weight_before_g\":"); json += before;
            json += F(",\"weight_after_g\":"); json += after;
            json += F(",\"consumed_g\":"); json += mass;
            json += F(",\"price_per_kg_minor\":"); json += price;
            json += F(",\"consumed_value_minor\":"); json += valueBuffer;
            json += F(",\"currency\":\""); json += jsonEscape(currency); json += '"';
            json += F(",\"timestamp\":\""); json += jsonEscape(timestamp); json += '"';
            if (hasComment)
            {
                json += F(",\"comment\":\""); json += jsonEscape(comment); json += '"';
            }
            json += '}';

            ++appendedCount;
            totalConsumedGrams += mass;
            totalValueMinor += consumedValueMinor;

            WriteOffMaterialTotals* totals = &materialTotals;
            uint32_t* gramsTarget = &totals->unknownGrams;
            uint64_t* valueTarget = &totals->unknownValueMinor;
            uint16_t* countTarget = &totals->unknownCount;
            if (wireType == "CU")
            {
                gramsTarget = &totals->copperGrams;
                valueTarget = &totals->copperValueMinor;
                countTarget = &totals->copperCount;
            }
            else if (wireType == "AL")
            {
                gramsTarget = &totals->aluminiumGrams;
                valueTarget = &totals->aluminiumValueMinor;
                countTarget = &totals->aluminiumCount;
            }

            if (*gramsTarget > 0xFFFFFFFFUL - mass ||
                *valueTarget > 0xFFFFFFFFFFFFFFFFULL - consumedValueMinor ||
                *countTarget == 0xFFFFU)
            {
                file.close();
                return false;
            }
            *gramsTarget += mass;
            *valueTarget += consumedValueMinor;
            ++(*countTarget);
        }

        pendingId = 0UL;
    }

    file.close();
    // WarehouseStore::begin() reconciles a dangling PENDING before m_ready=true.
    // Seeing one here therefore means runtime corruption or storage loss.
    return pendingId == 0UL;
}
}
