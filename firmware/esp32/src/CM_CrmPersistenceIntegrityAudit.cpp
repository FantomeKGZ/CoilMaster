#include "CM_CrmPersistenceIntegrityAudit.h"

#include <Arduino.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
constexpr const char* WindingVersionsPath = "/data/workshop/motor-winding-versions.ndjson";
constexpr const char* AsReceivedPath = "/data/workshop/repair-as-received.ndjson";
constexpr const char* MaterialRequestsPath = "/data/workshop/material-requests.ndjson";
constexpr const char* MaterialRequestMovementsPath =
    "/data/workshop/material-request-movements.ndjson";
constexpr const char* MaterialRequestStatusPath =
    "/data/workshop/material-request-status.ndjson";
constexpr uint8_t ReferenceBatchSize = 24U;

struct Reference
{
    uint32_t id = 0UL;
    bool found = false;
};

bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U]))
        return false;
    uint32_t parsed = 0UL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}'))
        return false;
    value = parsed;
    return true;
}

bool findOptionalUnsigned(const String& line,
                          const char* key,
                          uint32_t& value,
                          bool& present)
{
    const String marker = String('"') + key + F("\":");
    present = line.indexOf(marker) >= 0;
    value = 0UL;
    return !present || findUnsigned(line, key, value);
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (ch == '"')
            return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (ch == '\\')
        {
            if (pos >= line.length()) return false;
            const char escaped = line[pos++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool findBoolean(const String& line, const char* key, bool& value)
{
    value = false;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (line.substring(pos, pos + 4U) == "true")
    {
        value = true;
        pos += 4U;
    }
    else if (line.substring(pos, pos + 5U) == "false")
    {
        pos += 5U;
    }
    else return false;
    return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
}

bool increment(uint32_t& value)
{
    if (value == 0xFFFFFFFFUL) return false;
    ++value;
    return true;
}

bool resolveReferences(fs::FS& storage,
                       const char* path,
                       const char* key,
                       Reference* refs,
                       uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(path)) return false;
    File file = storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    uint8_t unresolved = count;
    while (file.available() && unresolved > 0U)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, key, candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t i = 0U; i < count; ++i)
        {
            if (!refs[i].found && refs[i].id == candidate)
            {
                refs[i].found = true;
                --unresolved;
            }
        }
    }
    file.close();
    return unresolved == 0U;
}

void resetReference(Reference& ref, uint32_t id)
{
    ref.id = id;
    ref.found = false;
}

bool validateWindingVersions(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(WindingVersionsPath)) return true;
    File file = storage.open(WindingVersionsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    Reference motorRefs[ReferenceBatchSize];
    Reference repairRefs[ReferenceBatchSize];
    Reference predecessorRefs[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    uint8_t repairCount = 0U;
    uint8_t predecessorCount = 0U;

    auto flush = [&]() -> bool
    {
        const bool ok = resolveReferences(storage, MotorsPath, "motor_id",
                                          motorRefs, batchCount) &&
                        resolveReferences(storage, RepairsPath, "repair_id",
                                          repairRefs, repairCount) &&
                        resolveReferences(storage, WindingVersionsPath,
                                          "winding_version_id",
                                          predecessorRefs, predecessorCount);
        batchCount = repairCount = predecessorCount = 0U;
        return ok;
    };

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, motorId = 0UL, workingRepeat = 0UL;
        uint32_t previousVersionId = 0UL, sourceRepairId = 0UL;
        bool previousPresent = false, repairPresent = false, startingPresent = false;
        String kind, createdAt, workingProgram;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "winding_version_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !findString(line, "version_kind", kind) || kind.length() == 0U ||
            !findString(line, "created_at", createdAt) || createdAt.length() < 10U ||
            !findString(line, "working_program", workingProgram) ||
            !WindingProgramParser::valid(workingProgram) ||
            !findUnsigned(line, "working_repeat_target", workingRepeat) || workingRepeat == 0UL ||
            !findBoolean(line, "starting_present", startingPresent) ||
            !findOptionalUnsigned(line, "previous_version_id", previousVersionId, previousPresent) ||
            !findOptionalUnsigned(line, "source_repair_id", sourceRepairId, repairPresent) ||
            (previousPresent && (previousVersionId == 0UL || previousVersionId >= id)) ||
            (repairPresent && sourceRepairId == 0UL) || !increment(count))
        {
            file.close();
            return false;
        }
        if (startingPresent)
        {
            String startingProgram;
            uint32_t startingRepeat = 0UL;
            if (!findString(line, "starting_program", startingProgram) ||
                !WindingProgramParser::valid(startingProgram) ||
                !findUnsigned(line, "starting_repeat_target", startingRepeat) ||
                startingRepeat == 0UL)
            {
                file.close();
                return false;
            }
        }
        previousId = id;
        resetReference(motorRefs[batchCount++], motorId);
        if (repairPresent) resetReference(repairRefs[repairCount++], sourceRepairId);
        if (previousPresent)
            resetReference(predecessorRefs[predecessorCount++], previousVersionId);
        if (batchCount == ReferenceBatchSize || repairCount == ReferenceBatchSize ||
            predecessorCount == ReferenceBatchSize)
        {
            if (!flush())
            {
                file.close();
                return false;
            }
        }
    }
    const bool ok = flush();
    file.close();
    return ok;
}

bool validateAsReceived(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(AsReceivedPath)) return true;
    File file = storage.open(AsReceivedPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousSnapshotId = 0UL, previousRepairId = 0UL;
    Reference repairRefs[ReferenceBatchSize], clientRefs[ReferenceBatchSize];
    Reference motorRefs[ReferenceBatchSize], versionRefs[ReferenceBatchSize];
    uint8_t batchCount = 0U, versionCount = 0U;

    auto flush = [&]() -> bool
    {
        const bool ok = resolveReferences(storage, RepairsPath, "repair_id", repairRefs, batchCount) &&
                        resolveReferences(storage, ClientsPath, "client_id", clientRefs, batchCount) &&
                        resolveReferences(storage, MotorsPath, "motor_id", motorRefs, batchCount) &&
                        resolveReferences(storage, WindingVersionsPath, "winding_version_id",
                                          versionRefs, versionCount);
        batchCount = versionCount = 0U;
        return ok;
    };

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t snapshotId = 0UL, repairId = 0UL, clientId = 0UL, motorId = 0UL;
        uint32_t versionId = 0UL, workingRepeat = 0UL;
        bool versionPresent = false, startingPresent = false;
        String capturedAt, sourceKind, motorName, workingProgram;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "snapshot_id", snapshotId) || snapshotId == 0UL ||
            snapshotId <= previousSnapshotId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            repairId <= previousRepairId ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !findOptionalUnsigned(line, "winding_version_id", versionId, versionPresent) ||
            (versionPresent && versionId == 0UL) ||
            !findString(line, "captured_at", capturedAt) || capturedAt.length() < 10U ||
            !findString(line, "source_kind", sourceKind) || sourceKind.length() == 0U ||
            !findString(line, "motor_name", motorName) || motorName.length() == 0U ||
            !findString(line, "working_program", workingProgram) ||
            !WindingProgramParser::valid(workingProgram) ||
            !findUnsigned(line, "working_repeat_target", workingRepeat) || workingRepeat == 0UL ||
            !findBoolean(line, "starting_present", startingPresent) || !increment(count))
        {
            file.close();
            return false;
        }
        if (startingPresent)
        {
            String program;
            uint32_t repeat = 0UL;
            if (!findString(line, "starting_program", program) ||
                !WindingProgramParser::valid(program) ||
                !findUnsigned(line, "starting_repeat_target", repeat) || repeat == 0UL)
            {
                file.close();
                return false;
            }
        }
        previousSnapshotId = snapshotId;
        previousRepairId = repairId;
        resetReference(repairRefs[batchCount], repairId);
        resetReference(clientRefs[batchCount], clientId);
        resetReference(motorRefs[batchCount], motorId);
        ++batchCount;
        if (versionPresent) resetReference(versionRefs[versionCount++], versionId);
        if (batchCount == ReferenceBatchSize || versionCount == ReferenceBatchSize)
        {
            if (!flush())
            {
                file.close();
                return false;
            }
        }
    }
    const bool ok = flush();
    file.close();
    return ok;
}

bool validateRequests(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(MaterialRequestsPath)) return true;
    File file = storage.open(MaterialRequestsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    Reference repairRefs[ReferenceBatchSize], clientRefs[ReferenceBatchSize];
    Reference motorRefs[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    auto flush = [&]() -> bool
    {
        const bool ok = resolveReferences(storage, RepairsPath, "repair_id", repairRefs, batchCount) &&
                        resolveReferences(storage, ClientsPath, "client_id", clientRefs, batchCount) &&
                        resolveReferences(storage, MotorsPath, "motor_id", motorRefs, batchCount);
        batchCount = 0U;
        return ok;
    };

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, repairId = 0UL, clientId = 0UL, motorId = 0UL;
        String status, createdAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_request_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !findString(line, "initial_status", status) || status != "DRAFT" ||
            !findString(line, "created_at", createdAt) || createdAt.length() < 10U ||
            !increment(count))
        {
            file.close();
            return false;
        }
        previousId = id;
        resetReference(repairRefs[batchCount], repairId);
        resetReference(clientRefs[batchCount], clientId);
        resetReference(motorRefs[batchCount], motorId);
        ++batchCount;
        if (batchCount == ReferenceBatchSize && !flush())
        {
            file.close();
            return false;
        }
    }
    const bool ok = flush();
    file.close();
    return ok;
}

bool validateMovements(fs::FS& storage, uint32_t& count)
{
    count = 0UL;
    if (!storage.exists(MaterialRequestMovementsPath)) return true;
    File file = storage.open(MaterialRequestMovementsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    Reference requestRefs[ReferenceBatchSize], repairRefs[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    auto flush = [&]() -> bool
    {
        const bool ok = resolveReferences(storage, MaterialRequestsPath,
                                          "material_request_id", requestRefs, batchCount) &&
                        resolveReferences(storage, RepairsPath, "repair_id",
                                          repairRefs, batchCount);
        batchCount = 0U;
        return ok;
    };

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, requestId = 0UL, repairId = 0UL, itemId = 0UL;
        uint32_t quantity = 0UL;
        String transactionRef, movementKind, sourceKind, unit, currency, createdAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "warehouse_item_id", itemId) || itemId == 0UL ||
            !findString(line, "transaction_ref", transactionRef) ||
            transactionRef.length() < 8U || transactionRef.length() > 80U ||
            !findString(line, "movement_kind", movementKind) ||
            (movementKind != "ISSUE" && movementKind != "RETURN" && movementKind != "CORRECTION") ||
            !findString(line, "source_kind", sourceKind) ||
            (sourceKind != "MANUAL_MATERIAL" && sourceKind != "RUN_WIRE") ||
            !findUnsigned(line, "quantity_milli_units", quantity) || quantity == 0UL ||
            !findString(line, "unit", unit) ||
            (unit != "KG" && unit != "L" && unit != "PCS" && unit != "M" && unit != "M2") ||
            (unit == "PCS" && quantity % 1000UL != 0UL) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "created_at", createdAt) || createdAt.length() < 10U ||
            !increment(count))
        {
            file.close();
            return false;
        }
        if (movementKind == "CORRECTION")
        {
            String correctionDirection;
            if (!findString(line, "correction_direction", correctionDirection) ||
                (correctionDirection != "ADD" && correctionDirection != "REMOVE"))
            {
                file.close();
                return false;
            }
        }
        else if (line.indexOf(F("\"correction_direction\":")) >= 0)
        {
            file.close();
            return false;
        }
        if (sourceKind == "RUN_WIRE")
        {
            uint32_t sessionId = 0UL, runId = 0UL, diameter = 0UL;
            String materialClass;
            if (movementKind != "ISSUE" || unit != "KG" ||
                !findUnsigned(line, "source_session_id", sessionId) || sessionId == 0UL ||
                !findUnsigned(line, "source_run_id", runId) || runId == 0UL ||
                !findString(line, "material_class", materialClass) ||
                (materialClass != "CU" && materialClass != "AL") ||
                !findUnsigned(line, "wire_diameter_hundredths_mm", diameter) ||
                diameter == 0UL || diameter > 500UL)
            {
                file.close();
                return false;
            }
        }
        previousId = id;
        resetReference(requestRefs[batchCount], requestId);
        resetReference(repairRefs[batchCount], repairId);
        ++batchCount;
        if (batchCount == ReferenceBatchSize && !flush())
        {
            file.close();
            return false;
        }
    }
    const bool ok = flush();
    file.close();
    return ok;
}

bool validateRequestStatuses(fs::FS& storage)
{
    if (!storage.exists(MaterialRequestStatusPath)) return true;
    File file = storage.open(MaterialRequestStatusPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousTransitionId = 0UL;
    Reference requestRefs[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    auto flush = [&]() -> bool
    {
        const bool ok = resolveReferences(storage, MaterialRequestsPath,
                                          "material_request_id",
                                          requestRefs, batchCount);
        batchCount = 0U;
        return ok;
    };
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t transitionId = 0UL, requestId = 0UL;
        String fromStatus, toStatus, changedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "transition_id", transitionId) || transitionId == 0UL ||
            transitionId <= previousTransitionId ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            !findString(line, "from_status", fromStatus) ||
            !findString(line, "to_status", toStatus) ||
            !findString(line, "changed_at", changedAt) || changedAt.length() < 10U ||
            changedAt.length() > 32U)
        {
            file.close();
            return false;
        }
        const bool legal =
            (fromStatus == "DRAFT" && toStatus == "ISSUED") ||
            (fromStatus == "ISSUED" && toStatus == "PRICED") ||
            (fromStatus == "PRICED" && toStatus == "CLOSED");
        if (!legal)
        {
            file.close();
            return false;
        }
        previousTransitionId = transitionId;
        resetReference(requestRefs[batchCount++], requestId);
        if (batchCount == ReferenceBatchSize && !flush())
        {
            file.close();
            return false;
        }
    }
    const bool ok = flush();
    file.close();
    return ok;
}
}

bool CrmPersistenceIntegrityAudit::check(fs::FS& storage)
{
    CrmPersistenceAuditMetrics metrics;
    return check(storage, metrics);
}

bool CrmPersistenceIntegrityAudit::check(fs::FS& storage,
                                         CrmPersistenceAuditMetrics& metrics)
{
    metrics = CrmPersistenceAuditMetrics();
    return validateWindingVersions(storage, metrics.windingVersionRecordCount) &&
           validateAsReceived(storage, metrics.asReceivedRecordCount) &&
           validateRequests(storage, metrics.materialRequestRecordCount) &&
           validateMovements(storage, metrics.materialRequestMovementRecordCount) &&
           validateRequestStatuses(storage);
}
}
