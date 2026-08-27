#include "CM_WarehouseStore.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_KgQuantity.h"

namespace CM
{
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
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", candidate) || candidate == 0UL ||
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
    return rewriteSpoolWeight(spoolId,
                              expectedWeightGrams,
                              newWeightGrams,
                              0U,
                              String(),
                              false,
                              diameterHundredthsMm,
                              wireType);
}

bool WarehouseStore::rewriteSpoolWeight(uint32_t spoolId,
                                         uint32_t expectedWeightGrams,
                                         uint32_t newWeightGrams,
                                         uint16_t expectedDiameterHundredthsMm,
                                         const String& expectedWireType,
                                         bool allowAlreadyApplied,
                                         uint16_t& diameterHundredthsMm,
                                         String& wireType)
{
    diameterHundredthsMm = 0U;
    wireType = String();
    if (spoolId == 0UL || expectedWeightGrams == 0UL || newWeightGrams == 0UL ||
        (expectedWireType.length() > 0U &&
         expectedWireType != "CU" && expectedWireType != "AL"))
    {
        return false;
    }

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
    bool alreadyApplied = false;
    uint32_t previousId = 0UL;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t currentWeight = 0UL;
        uint32_t diameter = 0UL;
        String status, currentWireType;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "current_weight_g", currentWeight) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findString(line, "status", status) || status.length() == 0U ||
            !findString(line, "wire_type", currentWireType) ||
            (currentWireType != "CU" && currentWireType != "AL"))
        {
            valid = false;
            break;
        }
        previousId = currentId;

        const char* optionalFields[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t index = 0U;
             index < sizeof(optionalFields) / sizeof(optionalFields[0]);
             ++index)
        {
            const String marker = String("\"") + optionalFields[index] + F("\":");
            if (line.indexOf(marker) >= 0)
            {
                String optional;
                if (!findString(line, optionalFields[index], optional))
                {
                    valid = false;
                    break;
                }
            }
        }
        if (!valid) break;

        if (currentId == spoolId)
        {
            if (found || status != "ACTIVE" ||
                (expectedDiameterHundredthsMm != 0U &&
                 diameter != expectedDiameterHundredthsMm) ||
                (expectedWireType.length() > 0U && currentWireType != expectedWireType))
            {
                valid = false;
                break;
            }

            const bool atBefore = currentWeight == expectedWeightGrams;
            const bool atAfter = allowAlreadyApplied && currentWeight == newWeightGrams;
            if (!atBefore && !atAfter)
            {
                valid = false;
                break;
            }

            if (atBefore)
            {
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
                if (!FlatJsonObjectValidator::valid(line) ||
                    !findUnsigned(line, "current_weight_g", verifiedWeight) ||
                    verifiedWeight != newWeightGrams)
                {
                    valid = false;
                    break;
                }
            }
            else
            {
                alreadyApplied = true;
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

    if (alreadyApplied)
    {
        // Replay is accepted only after the complete authoritative spool file
        // was validated. No rename is needed because the exact after-state is
        // already durable; discard the redundant temp copy.
        return m_storage.remove(SpoolsTempPath);
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

    // A run without a session is never valid. A session without a run is
    // accepted only so startup recovery can close a historical session-level
    // PENDING created before run-level provenance was introduced.
    if (movementId == 0UL || operation.spoolId == 0UL || operation.repairId == 0UL ||
        (operation.sourceRunId != 0UL && operation.sourceSessionId == 0UL) ||
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
    line.reserve(520U);
    line = F("{\"movement_id\":"); line += movementId;
    line += F(",\"type\":\"WRITE_OFF\",\"status\":\""); line += statusText;
    line += F("\",\"spool_id\":"); line += operation.spoolId;
    line += F(",\"repair_id\":"); line += operation.repairId;
    if (operation.sourceSessionId != 0UL)
    {
        line += F(",\"source_session_id\":"); line += operation.sourceSessionId;
        if (operation.sourceRunId != 0UL)
        {
            line += F(",\"source_run_id\":"); line += operation.sourceRunId;
        }
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

bool WarehouseStore::appendKgFirstWriteOffRecord(uint32_t movementId,
                                                  const KgFirstWriteOff& operation,
                                                  uint32_t weightBeforeGrams,
                                                  uint32_t weightAfterGrams,
                                                  const WarehousePrice& price,
                                                  const char* status)
{
    if (status == nullptr || movementId == 0UL || operation.repairId == 0UL ||
        operation.sourceSessionId == 0UL || operation.sourceRunId == 0UL ||
        operation.diameterHundredthsMm == 0U || operation.consumedGrams == 0UL ||
        (operation.wireType != "CU" && operation.wireType != "AL") ||
        operation.timestamp.length() < 10U || price.pricePerKgMinor == 0UL ||
        price.currency.length() != 3U)
    {
        return false;
    }
    const String statusText(status);
    if (statusText != "PENDING" && statusText != "CONFIRMED" && statusText != "ABORTED")
        return false;

    const bool stockManaged = operation.spoolId != 0UL;
    if (stockManaged)
    {
        if (weightBeforeGrams == 0UL || weightAfterGrams >= weightBeforeGrams ||
            operation.consumedGrams != weightBeforeGrams - weightAfterGrams)
            return false;
    }
    else if (weightBeforeGrams != 0UL || weightAfterGrams != 0UL)
    {
        return false;
    }

    const String quantityKg = KgQuantity::canonicalKg(operation.consumedGrams);
    if (quantityKg.length() == 0U) return false;

    File file = m_storage.open(MovementsPath, FILE_APPEND);
    if (!file) return false;

    String line;
    line.reserve(620U);
    line = F("{\"movement_id\":"); line += movementId;
    line += F(",\"type\":\"WRITE_OFF\",\"status\":\""); line += statusText;
    line += F("\",\"writeoff_mode\":\"KG_FIRST\",\"stock_mode\":\"");
    line += stockManaged ? F("SPOOL") : F("UNALLOCATED");
    line += '"';
    if (stockManaged)
    {
        line += F(",\"spool_id\":"); line += operation.spoolId;
    }
    line += F(",\"repair_id\":"); line += operation.repairId;
    line += F(",\"source_session_id\":"); line += operation.sourceSessionId;
    line += F(",\"source_run_id\":"); line += operation.sourceRunId;
    line += F(",\"diameter_hundredths_mm\":"); line += operation.diameterHundredthsMm;
    line += F(",\"wire_type\":\""); line += jsonEscape(operation.wireType); line += '"';
    if (stockManaged)
    {
        line += F(",\"weight_before_g\":"); line += weightBeforeGrams;
        line += F(",\"weight_after_g\":"); line += weightAfterGrams;
    }
    line += F(",\"mass_g\":"); line += operation.consumedGrams;
    line += F(",\"quantity_kg\":\""); line += quantityKg; line += '"';
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
