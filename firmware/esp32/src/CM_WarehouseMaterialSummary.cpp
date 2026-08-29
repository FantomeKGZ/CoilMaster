#include "CM_WarehouseStore.h"

namespace CM
{
namespace
{
struct MaterialDiameterSummary
{
    uint16_t diameter;
    uint32_t remaining;
    uint16_t spoolCount;
    uint32_t consumedMonth;
    uint32_t consumedAll;

    MaterialDiameterSummary()
        : diameter(0U), remaining(0UL), spoolCount(0U),
          consumedMonth(0UL), consumedAll(0UL) {}
};

struct MaterialSummary
{
    const char* code;
    MaterialDiameterSummary diameters[WarehouseMaxDiameters];
    uint8_t count;
    uint32_t remaining;
    uint32_t spoolCount;
    uint32_t consumedMonth;
    uint32_t consumedAll;

    MaterialSummary()
        : code("UNKNOWN"), count(0U), remaining(0UL), spoolCount(0UL),
          consumedMonth(0UL), consumedAll(0UL) {}
};

uint8_t materialIndex(const String& value)
{
    if (value == "CU") return 0U;
    if (value == "AL") return 1U;
    return 2U;
}

MaterialDiameterSummary* findOrAdd(MaterialSummary& summary, uint16_t diameter)
{
    for (uint8_t i = 0U; i < summary.count; ++i)
        if (summary.diameters[i].diameter == diameter) return &summary.diameters[i];
    if (summary.count >= WarehouseMaxDiameters) return nullptr;
    MaterialDiameterSummary& item = summary.diameters[summary.count++];
    item.diameter = diameter;
    return &item;
}

void sortDiameters(MaterialSummary& summary)
{
    for (uint8_t i = 0U; i < summary.count; ++i)
        for (uint8_t j = static_cast<uint8_t>(i + 1U); j < summary.count; ++j)
            if (summary.diameters[j].diameter < summary.diameters[i].diameter)
            {
                const MaterialDiameterSummary temp = summary.diameters[i];
                summary.diameters[i] = summary.diameters[j];
                summary.diameters[j] = temp;
            }
}
}

bool WarehouseStore::appendMaterialSummaryJson(String& json,
                                                const char* monthPrefix) const
{
    if (!ready() || monthPrefix == nullptr) return false;

    MaterialSummary groups[3];
    groups[0].code = "CU";
    groups[1].code = "AL";
    groups[2].code = "UNKNOWN";

    if (m_storage.exists(SpoolsPath))
    {
        File spools = m_storage.open(SpoolsPath, FILE_READ);
        if (!spools) return false;
        uint32_t previousSpoolId = 0UL;
        while (spools.available())
        {
            const String line = spools.readStringUntil('\n');
            if (line.length() == 0U) continue;

            uint32_t spoolId = 0UL;
            uint32_t diameter = 0UL;
            uint32_t weight = 0UL;
            String status, wireType;
            if (!findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
                spoolId <= previousSpoolId ||
                !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                diameter == 0UL || diameter > 0xFFFFUL ||
                !findUnsigned(line, "current_weight_g", weight) ||
                !findString(line, "status", status))
            {
                spools.close();
                return false;
            }
            previousSpoolId = spoolId;

            const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
            if (hasWireType &&
                (!findString(line, "wire_type", wireType) ||
                 (wireType != "CU" && wireType != "AL")))
            {
                spools.close();
                return false;
            }

            if (status != "ACTIVE") continue;

            MaterialSummary& group = groups[materialIndex(wireType)];
            MaterialDiameterSummary* item =
                findOrAdd(group, static_cast<uint16_t>(diameter));
            if (item == nullptr ||
                item->remaining > 0xFFFFFFFFUL - weight ||
                item->spoolCount == 0xFFFFU ||
                group.remaining > 0xFFFFFFFFUL - weight ||
                group.spoolCount == 0xFFFFFFFFUL)
            {
                spools.close();
                return false;
            }
            item->remaining += weight;
            ++item->spoolCount;
            group.remaining += weight;
            ++group.spoolCount;
        }
        spools.close();
    }

    if (m_storage.exists(MovementsPath))
    {
        File movements = m_storage.open(MovementsPath, FILE_READ);
        if (!movements) return false;

        uint32_t maximumMovementId = 0UL;
        uint32_t pendingId = 0UL;
        uint32_t pendingSpoolId = 0UL;
        uint32_t pendingRepairId = 0UL;
        uint32_t pendingBefore = 0UL;
        uint32_t pendingAfter = 0UL;
        uint32_t pendingMass = 0UL;
        uint32_t pendingPrice = 0UL;
        String pendingCurrency, pendingTimestamp, pendingComment;

        while (movements.available())
        {
            const String line = movements.readStringUntil('\n');
            if (line.length() == 0U) continue;

            uint32_t movementId = 0UL;
            uint32_t spoolId = 0UL;
            uint32_t repairId = 0UL;
            uint32_t diameter = 0UL;
            uint32_t before = 0UL;
            uint32_t after = 0UL;
            uint32_t grams = 0UL;
            uint32_t price = 0UL;
            String type, status, timestamp, wireType, currency, comment;

            if (!findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
                !findString(line, "type", type) || type != "WRITE_OFF" ||
                !findString(line, "status", status) ||
                !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
                !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
                !findUnsigned(line, "diameter_hundredths_mm", diameter) || diameter > 0xFFFFUL ||
                !findUnsigned(line, "weight_before_g", before) || before == 0UL ||
                !findUnsigned(line, "weight_after_g", after) || after >= before ||
                !findUnsigned(line, "mass_g", grams) || grams != before - after ||
                !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
                !findString(line, "currency", currency) || currency.length() != 3U ||
                !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
            {
                movements.close();
                return false;
            }

            const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
            if (hasWireType &&
                (!findString(line, "wire_type", wireType) ||
                 (wireType != "CU" && wireType != "AL")))
            {
                movements.close();
                return false;
            }
            const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
            if (hasComment && !findString(line, "comment", comment))
            {
                movements.close();
                return false;
            }

            if (status == "PENDING")
            {
                if (pendingId != 0UL || movementId <= maximumMovementId ||
                    diameter != 0UL || hasWireType)
                {
                    movements.close();
                    return false;
                }
                maximumMovementId = movementId;
                pendingId = movementId;
                pendingSpoolId = spoolId;
                pendingRepairId = repairId;
                pendingBefore = before;
                pendingAfter = after;
                pendingMass = grams;
                pendingPrice = price;
                pendingCurrency = currency;
                pendingTimestamp = timestamp;
                pendingComment = comment;
                continue;
            }

            if (status != "CONFIRMED" && status != "ABORTED")
            {
                movements.close();
                return false;
            }
            if (pendingId == 0UL || movementId != pendingId ||
                spoolId != pendingSpoolId || repairId != pendingRepairId ||
                before != pendingBefore || after != pendingAfter || grams != pendingMass ||
                price != pendingPrice || currency != pendingCurrency ||
                timestamp != pendingTimestamp || comment != pendingComment)
            {
                movements.close();
                return false;
            }

            if (status == "ABORTED")
            {
                if (diameter != 0UL || hasWireType)
                {
                    movements.close();
                    return false;
                }
                pendingId = 0UL;
                continue;
            }

            if (diameter == 0UL)
            {
                movements.close();
                return false;
            }

            if (!hasWireType && m_storage.exists(SpoolsPath))
            {
                File spools = m_storage.open(SpoolsPath, FILE_READ);
                if (!spools)
                {
                    movements.close();
                    return false;
                }
                uint32_t previousSpoolId = 0UL;
                while (spools.available())
                {
                    const String spoolLine = spools.readStringUntil('\n');
                    if (spoolLine.length() == 0U) continue;
                    uint32_t currentId = 0UL;
                    uint32_t currentDiameter = 0UL;
                    uint32_t currentWeight = 0UL;
                    String spoolStatus, currentWireType;
                    if (!findUnsigned(spoolLine, "spool_id", currentId) || currentId == 0UL ||
                        currentId <= previousSpoolId ||
                        !findUnsigned(spoolLine, "diameter_hundredths_mm", currentDiameter) ||
                        currentDiameter == 0UL || currentDiameter > 0xFFFFUL ||
                        !findUnsigned(spoolLine, "current_weight_g", currentWeight) ||
                        !findString(spoolLine, "status", spoolStatus))
                    {
                        spools.close();
                        movements.close();
                        return false;
                    }
                    previousSpoolId = currentId;

                    const bool spoolHasWireType = spoolLine.indexOf(F("\"wire_type\":")) >= 0;
                    if (spoolHasWireType &&
                        (!findString(spoolLine, "wire_type", currentWireType) ||
                         (currentWireType != "CU" && currentWireType != "AL")))
                    {
                        spools.close();
                        movements.close();
                        return false;
                    }

                    // The primary spool pass above already validated the complete,
                    // strictly increasing spool snapshot. No write occurs before this
                    // legacy read-only lookup, so once the requested id is reached (or
                    // passed) reading the remaining growing file cannot change the result.
                    if (currentId < spoolId) continue;
                    if (currentId == spoolId && spoolHasWireType)
                        wireType = currentWireType;
                    break;
                }
                spools.close();
            }

            MaterialSummary& group = groups[materialIndex(wireType)];
            MaterialDiameterSummary* item =
                findOrAdd(group, static_cast<uint16_t>(diameter));
            if (item == nullptr ||
                item->consumedAll > 0xFFFFFFFFUL - grams ||
                group.consumedAll > 0xFFFFFFFFUL - grams)
            {
                movements.close();
                return false;
            }
            item->consumedAll += grams;
            group.consumedAll += grams;

            if (timestamp.startsWith(monthPrefix))
            {
                if (item->consumedMonth > 0xFFFFFFFFUL - grams ||
                    group.consumedMonth > 0xFFFFFFFFUL - grams)
                {
                    movements.close();
                    return false;
                }
                item->consumedMonth += grams;
                group.consumedMonth += grams;
            }
            pendingId = 0UL;
        }
        movements.close();
        if (pendingId != 0UL) return false;
    }

    for (uint8_t i = 0U; i < 3U; ++i) sortDiameters(groups[i]);

    json += F("\"materials\":[");
    for (uint8_t groupIndex = 0U; groupIndex < 3U; ++groupIndex)
    {
        if (groupIndex > 0U) json += ',';
        const MaterialSummary& group = groups[groupIndex];
        json += F("{\"material\":\""); json += group.code;
        json += F("\",\"remaining_g\":"); json += group.remaining;
        json += F(",\"active_spools\":"); json += group.spoolCount;
        json += F(",\"consumed_month_g\":"); json += group.consumedMonth;
        json += F(",\"consumed_all_time_g\":"); json += group.consumedAll;
        json += F(",\"diameter_count\":"); json += group.count;
        json += F(",\"diameters\":[");
        for (uint8_t i = 0U; i < group.count; ++i)
        {
            if (i > 0U) json += ',';
            const MaterialDiameterSummary& item = group.diameters[i];
            json += F("{\"diameter_hundredths_mm\":"); json += item.diameter;
            json += F(",\"remaining_g\":"); json += item.remaining;
            json += F(",\"active_spools\":"); json += item.spoolCount;
            json += F(",\"consumed_month_g\":"); json += item.consumedMonth;
            json += F(",\"consumed_all_time_g\":"); json += item.consumedAll;
            json += '}';
        }
        json += F("]}");
    }
    json += F("]");
    return true;
}
}