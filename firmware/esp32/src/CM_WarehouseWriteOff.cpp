#include "CM_WarehouseStore.h"
#include "CM_RepairLifecycle.h"
#include "CM_WindingSessionCompletionAudit.h"

namespace CM
{
bool WarehouseStore::confirmSpoolWriteOff(const ConfirmedSpoolWriteOff& operation,
                                           SpoolWriteOffResult& result)
{
    result = SpoolWriteOffResult();
    if (!ready() || operation.spoolId == 0UL || operation.repairId == 0UL ||
        operation.timestamp.length() < 10U || operation.weightBeforeGrams == 0UL ||
        operation.weightAfterGrams >= operation.weightBeforeGrams)
    {
        return false;
    }

    bool repairFound = false;
    if (!repairExists(operation.repairId, repairFound) || !repairFound)
    {
        return false;
    }

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_storage, operation.repairId, repairOpen) ||
        !repairOpen)
    {
        return false;
    }

    if (operation.sourceSessionId != 0UL)
    {
        if (WindingSessionCompletionAudit::check(m_storage,
                                                 operation.sourceSessionId) !=
            WindingSessionCompletionCheck::Completed)
        {
            return false;
        }

        bool alreadyConfirmed = false;
        if (!confirmedWriteOffForSourceSession(operation.sourceSessionId,
                                               alreadyConfirmed) ||
            alreadyConfirmed)
        {
            return false;
        }
    }

    WarehousePrice price;
    bool priceConfigured = false;
    if (!loadWarehousePrice(price, priceConfigured) || !priceConfigured) return false;

    uint32_t movementId = 0UL;
    if (!nextMovementId(movementId)) return false;

    const uint32_t consumed = operation.weightBeforeGrams - operation.weightAfterGrams;
    uint16_t diameter = 0U;
    String wireType;

    // PENDING is ignored by statistics until the spool mutation is durable.
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
        // The persisted state is now ambiguous until startup reconciliation.
        m_ready = false;
        return false;
    }

    if (!appendWriteOffRecord(movementId, operation, diameter, consumed, price,
                              "CONFIRMED", wireType))
    {
        uint16_t ignoredDiameter = 0U;
        String ignoredWireType;
        if (!rewriteSpoolWeight(operation.spoolId,
                                operation.weightAfterGrams,
                                operation.weightBeforeGrams,
                                ignoredDiameter,
                                ignoredWireType))
        {
            m_ready = false;
            return false;
        }

        // The stock mutation was successfully rolled back. Close the append-only
        // transaction explicitly so the next movement id is not blocked.
        if (!appendWriteOffRecord(movementId, operation, 0U, consumed, price,
                                  "ABORTED", String()))
        {
            m_ready = false;
        }
        return false;
    }

    result.movementId = movementId;
    result.diameterHundredthsMm = diameter;
    result.consumedGrams = consumed;
    result.pricePerKgMinor = price.pricePerKgMinor;
    result.currency = price.currency;
    result.wireType = wireType;
    return true;
}

bool WarehouseStore::nextMovementId(uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(MovementsPath)) return true;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximum = 0UL;
    uint32_t openPendingId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidate = 0UL;
        String type, status;
        if (!findUnsigned(line, "movement_id", candidate) || candidate == 0UL ||
            !findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status))
        {
            file.close();
            return false;
        }

        if (status == "PENDING")
        {
            if (openPendingId != 0UL || candidate <= maximum)
            {
                file.close();
                return false;
            }
            openPendingId = candidate;
            maximum = candidate;
        }
        else if (status == "CONFIRMED" || status == "ABORTED")
        {
            if (openPendingId == 0UL || candidate != openPendingId)
            {
                file.close();
                return false;
            }
            openPendingId = 0UL;
        }
        else
        {
            file.close();
            return false;
        }
    }
    file.close();

    // A dangling PENDING requires reconciliation before another movement ID
    // may be issued. Never hide an incomplete stock transaction by moving on.
    if (openPendingId != 0UL || maximum == 0xFFFFFFFFUL) return false;
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
    uint32_t previousId = 0UL;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t currentWeight = 0UL;
        uint32_t diameter = 0UL;
        String status, currentWireType;
        if (!findUnsigned(line, "spool_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "current_weight_g", currentWeight) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findString(line, "status", status) ||
            !findString(line, "wire_type", currentWireType) ||
            (currentWireType != "CU" && currentWireType != "AL"))
        {
            valid = false;
            break;
        }
        previousId = currentId;

        if (currentId == spoolId)
        {
            if (found || currentWeight != expectedWeightGrams || status != "ACTIVE")
            {
                valid = false;
                break;
            }

            const String marker = F("\"current_weight_g\":");
            const int valueStart = line.indexOf(marker);
            if (valueStart < 0)
            {
                valid = false;
                break;
            }
            int digitsStart = valueStart + marker.length();
            int digitsEnd = digitsStart;
            while (digitsEnd < line.length() && isDigit(line[digitsEnd])) ++digitsEnd;
            line = line.substring(0, digitsStart) + String(newWeightGrams) +
                   line.substring(digitsEnd);

            uint32_t verifiedWeight = 0UL;
            if (!findUnsigned(line, "current_weight_g", verifiedWeight) ||
                verifiedWeight != newWeightGrams)
            {
                valid = false;
                break;
            }
            diameterHundredthsMm = static_cast<uint16_t>(diameter);
            wireType = currentWireType;
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

    return replaceSpoolsFileFromTemp();
}

bool WarehouseStore::appendWriteOffRecord(uint32_t movementId,
                                           const ConfirmedSpoolWriteOff& operation,
                                           uint16_t diameterHundredthsMm,
                                           uint32_t consumedGrams,
                                           const WarehousePrice& price,
                                           const char* status,
                                           const String& wireType)
{
    if (status == nullptr) return false;
    const String statusText(status);
    const bool isPendingLike = statusText == "PENDING" || statusText == "ABORTED";
    const bool isConfirmed = statusText == "CONFIRMED";

    if (movementId == 0UL || operation.spoolId == 0UL || operation.repairId == 0UL ||
        consumedGrams == 0UL || price.pricePerKgMinor == 0UL ||
        price.currency.length() != 3U || (!isPendingLike && !isConfirmed) ||
        (isPendingLike && (diameterHundredthsMm != 0U || wireType.length() != 0U)) ||
        (isConfirmed &&
         (diameterHundredthsMm == 0U || (wireType != "CU" && wireType != "AL"))))
    {
        return false;
    }

    File file = m_storage.open(MovementsPath, FILE_APPEND);
    if (!file) return false;

    String line;
    line.reserve(490U);
    line = F("{\"movement_id\":"); line += movementId;
    line += F(",\"type\":\"WRITE_OFF\",\"status\":\""); line += statusText;
    line += F("\",\"spool_id\":"); line += operation.spoolId;
    line += F(",\"repair_id\":"); line += operation.repairId;
    if (operation.sourceSessionId != 0UL)
    {
        line += F(",\"source_session_id\":"); line += operation.sourceSessionId;
    }
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
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}
}
