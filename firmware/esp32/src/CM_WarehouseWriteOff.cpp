#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::confirmSpoolWriteOff(const ConfirmedSpoolWriteOff& operation,
                                           SpoolWriteOffResult& result)
{
    result = SpoolWriteOffResult();
    if (!m_ready || operation.spoolId == 0UL || operation.repairId == 0UL ||
        operation.timestamp.length() < 10U || operation.weightBeforeGrams == 0UL ||
        operation.weightAfterGrams >= operation.weightBeforeGrams)
    {
        return false;
    }

    WarehousePrice price;
    if (!loadWarehousePrice(price)) return false;

    uint32_t movementId = 0UL;
    if (!nextMovementId(movementId)) return false;

    const uint32_t consumed = operation.weightBeforeGrams - operation.weightAfterGrams;
    uint16_t diameter = 0U;
    String wireType;

    // PENDING is ignored by statistics. Material is written on CONFIRMED.
    if (!appendWriteOffRecord(movementId, operation, 0U, consumed, price,
                              "PENDING", String()))
    {
        return false;
    }

    if (!rewriteSpoolWeight(operation.spoolId,
                            operation.weightBeforeGrams,
                            operation.weightAfterGrams,
                            diameter,
                            wireType))
    {
        return false;
    }

    if (!appendWriteOffRecord(movementId, operation, diameter, consumed, price,
                              "CONFIRMED", wireType))
    {
        uint16_t ignoredDiameter = 0U;
        String ignoredWireType;
        rewriteSpoolWeight(operation.spoolId,
                           operation.weightAfterGrams,
                           operation.weightBeforeGrams,
                           ignoredDiameter,
                           ignoredWireType);
        return false;
    }

    result.movementId = movementId;
    result.diameterHundredthsMm = diameter;
    result.consumedGrams = consumed;
    result.pricePerKgMinor = price.pricePerKgMinor;
    result.currency = price.currency;
    return true;
}

bool WarehouseStore::nextMovementId(uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximum = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t candidate = 0UL;
        if (findUnsigned(line, "movement_id", candidate) && candidate > maximum)
            maximum = candidate;
    }
    file.close();
    if (maximum == 0xFFFFFFFFUL) return false;
    id = maximum + 1UL;
    return true;
}

bool WarehouseStore::rewriteSpoolWeight(uint32_t spoolId,
                                         uint32_t expectedWeightGrams,
                                         uint32_t newWeightGrams,
                                         uint16_t& diameterHundredthsMm,
                                         String& wireType)
{
    diameterHundredthsMm = 0U;
    wireType = String();
    File source = m_storage.open(SpoolsPath, FILE_READ);
    if (!source) return false;

    m_storage.remove(SpoolsTempPath);
    File target = m_storage.open(SpoolsTempPath, FILE_WRITE);
    if (!target)
    {
        source.close();
        return false;
    }

    bool found = false;
    bool valid = true;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        uint32_t currentId = 0UL;
        if (findUnsigned(line, "spool_id", currentId) && currentId == spoolId)
        {
            uint32_t currentWeight = 0UL;
            uint32_t diameter = 0UL;
            String status;
            findString(line, "status", status);
            if (!findUnsigned(line, "current_weight_g", currentWeight) ||
                !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                currentWeight != expectedWeightGrams ||
                (status.length() > 0U && status != "ACTIVE"))
            {
                valid = false;
                break;
            }

            const String marker = F("\"current_weight_g\":");
            const int valueStart = line.indexOf(marker);
            int digitsStart = valueStart + marker.length();
            int digitsEnd = digitsStart;
            while (digitsEnd < line.length() && isDigit(line[digitsEnd])) ++digitsEnd;
            line = line.substring(0, digitsStart) + String(newWeightGrams) +
                   line.substring(digitsEnd);
            diameterHundredthsMm = static_cast<uint16_t>(diameter);
            findString(line, "wire_type", wireType);
            found = true;
        }

        if (target.println(line) == 0U)
        {
            valid = false;
            break;
        }
    }

    source.close();
    target.flush();
    target.close();

    if (!valid || !found)
    {
        m_storage.remove(SpoolsTempPath);
        return false;
    }

    m_storage.remove(SpoolsPath);
    return m_storage.rename(SpoolsTempPath, SpoolsPath);
}

bool WarehouseStore::appendWriteOffRecord(uint32_t movementId,
                                           const ConfirmedSpoolWriteOff& operation,
                                           uint16_t diameterHundredthsMm,
                                           uint32_t consumedGrams,
                                           const WarehousePrice& price,
                                           const char* status,
                                           const String& wireType)
{
    File file = m_storage.open(MovementsPath, FILE_APPEND);
    if (!file) return false;

    String line;
    line.reserve(450U);
    line = F("{\"movement_id\":"); line += movementId;
    line += F(",\"type\":\"WRITE_OFF\",\"status\":\""); line += status;
    line += F("\",\"spool_id\":"); line += operation.spoolId;
    line += F(",\"repair_id\":"); line += operation.repairId;
    line += F(",\"diameter_hundredths_mm\":"); line += diameterHundredthsMm;
    if (wireType.length() > 0U)
    {
        line += F(",\"wire_type\":\""); line += jsonEscape(wireType); line += '"';
    }
    line += F(",\"weight_before_g\":"); line += operation.weightBeforeGrams;
    line += F(",\"weight_after_g\":"); line += operation.weightAfterGrams;
    line += F(",\"mass_g\":"); line += consumedGrams;
    line += F(",\"price_per_kg_minor\":"); line += price.pricePerKgMinor;
    line += F(",\"currency\":\""); line += jsonEscape(price.currency);
    line += F("\",\"timestamp\":\""); line += jsonEscape(operation.timestamp); line += '"';
    if (operation.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(operation.comment); line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    return written == line.length();
}
}
