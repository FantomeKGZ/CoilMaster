#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::recoverPendingWriteOff()
{
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximumId = 0UL;
    uint32_t pendingId = 0UL;
    ConfirmedSpoolWriteOff pendingOperation;
    WarehousePrice pendingPrice;
    uint32_t pendingConsumed = 0UL;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t movementId = 0UL;
        uint32_t spoolId = 0UL;
        uint32_t repairId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t before = 0UL;
        uint32_t after = 0UL;
        uint32_t mass = 0UL;
        uint32_t price = 0UL;
        String type, status, currency, timestamp, comment, wireType;

        if (!findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            !findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
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

        const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
        if (hasComment && !findString(line, "comment", comment))
        {
            file.close();
            return false;
        }
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (hasWireType && !findString(line, "wire_type", wireType))
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
            pendingOperation.spoolId = spoolId;
            pendingOperation.repairId = repairId;
            pendingOperation.weightBeforeGrams = before;
            pendingOperation.weightAfterGrams = after;
            pendingOperation.timestamp = timestamp;
            pendingOperation.comment = comment;
            pendingPrice.pricePerKgMinor = price;
            pendingPrice.currency = currency;
            pendingConsumed = mass;
            continue;
        }

        if (status != "CONFIRMED" && status != "ABORTED")
        {
            file.close();
            return false;
        }

        if (pendingId == 0UL || movementId != pendingId ||
            spoolId != pendingOperation.spoolId ||
            repairId != pendingOperation.repairId ||
            before != pendingOperation.weightBeforeGrams ||
            after != pendingOperation.weightAfterGrams ||
            mass != pendingConsumed ||
            price != pendingPrice.pricePerKgMinor ||
            currency != pendingPrice.currency ||
            timestamp != pendingOperation.timestamp ||
            comment != pendingOperation.comment)
        {
            file.close();
            return false;
        }

        if (status == "CONFIRMED")
        {
            if (diameter == 0UL ||
                (hasWireType && wireType != "CU" && wireType != "AL"))
            {
                file.close();
                return false;
            }
        }
        else if (diameter != 0UL || hasWireType)
        {
            file.close();
            return false;
        }

        pendingId = 0UL;
        pendingOperation = ConfirmedSpoolWriteOff();
        pendingPrice = WarehousePrice();
        pendingConsumed = 0UL;
    }
    file.close();

    if (pendingId == 0UL) return true;

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

        if (spoolId != pendingOperation.spoolId) continue;
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

    if (currentWeight == pendingOperation.weightBeforeGrams)
    {
        return appendWriteOffRecord(pendingId,
                                    pendingOperation,
                                    0U,
                                    pendingConsumed,
                                    pendingPrice,
                                    "ABORTED",
                                    String());
    }

    if (currentWeight == pendingOperation.weightAfterGrams)
    {
        return appendWriteOffRecord(pendingId,
                                    pendingOperation,
                                    diameter,
                                    pendingConsumed,
                                    pendingPrice,
                                    "CONFIRMED",
                                    wireType);
    }

    return false;
}
}
