#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"

namespace CM
{
namespace
{
constexpr const char* WebStateDirectory = "/data/winding-jobs/state";

struct WebJobPageItem
{
    uint32_t sessionId;
    WebJobPageItem() : sessionId(0UL) {}
};

struct WebJobAssignment
{
    uint32_t assignmentId;
    uint32_t sessionId;
    uint32_t runId;
    uint32_t motorId;
    String role;

    WebJobAssignment()
        : assignmentId(0UL), sessionId(0UL), runId(0UL), motorId(0UL), role() {}
};

bool parseStateFileName(const String& name, uint32_t expectedSessionId)
{
    const int slash = name.lastIndexOf('/');
    const String base = slash >= 0 ? name.substring(slash + 1) : name;
    return base == String(F("session-")) + expectedSessionId + F(".json");
}

String snapshotProgram(const JobSnapshot& snapshot)
{
    String result;
    result.reserve(static_cast<unsigned int>(snapshot.coilCount) * 6U);
    for (uint8_t index = 0U; index < snapshot.coilCount; ++index)
    {
        if (index > 0U) result += '/';
        result += snapshot.turns[index];
    }
    return result;
}
}

bool AutonomousWindingArchive::appendCompletedWebJobsPageJson(
    String& json,
    uint32_t cursorSessionId,
    uint8_t limit,
    uint16_t& count,
    uint32_t& nextCursorSessionId,
    bool& hasMore) const
{
    count = 0U;
    nextCursorSessionId = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxTaskPageSize) return false;
    if (!m_storage.exists(WebStateDirectory)) return cursorSessionId == 0UL;

    File directory = m_storage.open(WebStateDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }

    // Keep only the lowest limit+1 completed sessions above the cursor. This
    // gives deterministic ascending paging without buffering the whole directory.
    WebJobPageItem selected[MaxTaskPageSize + 1U];
    uint8_t selectedCount = 0U;
    File entry = directory.openNextFile();
    while (entry)
    {
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return false;
        }

        const String name = entry.name();
        entry.close();

        // A state directory with a temporary/backup/unexpected file is an
        // integrity problem. Historical listing must fail closed like loadLatest.
        if (!name.endsWith(F(".json")))
        {
            directory.close();
            return false;
        }

        uint32_t parsedSessionId = 0UL;
        const int slash = name.lastIndexOf('/');
        const String base = slash >= 0 ? name.substring(slash + 1) : name;
        if (!base.startsWith(F("session-")) || !base.endsWith(F(".json")))
        {
            directory.close();
            return false;
        }
        const String idText = base.substring(8, base.length() - 5);
        if (idText.length() == 0U || (idText.length() > 1U && idText[0] == '0'))
        {
            directory.close();
            return false;
        }
        for (size_t index = 0U; index < idText.length(); ++index)
        {
            if (!isDigit(idText[index]))
            {
                directory.close();
                return false;
            }
            const uint8_t digit = static_cast<uint8_t>(idText[index] - '0');
            if (parsedSessionId > (0xFFFFFFFFUL - digit) / 10UL)
            {
                directory.close();
                return false;
            }
            parsedSessionId = parsedSessionId * 10UL + digit;
        }
        if (parsedSessionId == 0UL)
        {
            directory.close();
            return false;
        }

        JobRuntimeState state;
        if (!JobStateStore::readPersisted(m_storage, parsedSessionId, state) ||
            !parseStateFileName(name, state.sessionId))
        {
            directory.close();
            return false;
        }
        if (state.executionState != JobExecutionState::ProgramCompleted ||
            state.sessionId <= cursorSessionId)
        {
            entry = directory.openNextFile();
            continue;
        }

        uint8_t insertAt = 0U;
        while (insertAt < selectedCount &&
               selected[insertAt].sessionId < state.sessionId)
            ++insertAt;
        if (insertAt < selectedCount &&
            selected[insertAt].sessionId == state.sessionId)
        {
            directory.close();
            return false;
        }

        const uint8_t capacity = MaxTaskPageSize + 1U;
        if (selectedCount < capacity)
        {
            for (uint8_t move = selectedCount; move > insertAt; --move)
                selected[move] = selected[move - 1U];
            selected[insertAt].sessionId = state.sessionId;
            ++selectedCount;
        }
        else if (insertAt < capacity)
        {
            for (uint8_t move = capacity - 1U; move > insertAt; --move)
                selected[move] = selected[move - 1U];
            selected[insertAt].sessionId = state.sessionId;
        }

        entry = directory.openNextFile();
    }
    directory.close();

    hasMore = selectedCount > limit;
    const uint8_t pageCount = hasMore ? limit : selectedCount;

    WebJobAssignment assignments[MaxTaskPageSize];
    bool assignmentFound[MaxTaskPageSize] = {};
    if (pageCount > 0U && m_storage.exists(WebAssignmentsPath))
    {
        File file = m_storage.open(WebAssignmentsPath, FILE_READ);
        if (!file || file.isDirectory())
        {
            if (file) file.close();
            return false;
        }

        uint32_t previousAssignmentId = 0UL;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;
            if (!FlatJsonObjectValidator::valid(line))
            {
                file.close();
                return false;
            }

            uint32_t schema = 0UL;
            WebJobAssignment assignment;
            String source;
            if (!findUnsigned(line, "schema_version", schema) || schema != 1UL ||
                !findUnsigned(line, "assignment_id", assignment.assignmentId) ||
                assignment.assignmentId == 0UL ||
                assignment.assignmentId <= previousAssignmentId ||
                !findString(line, "source", source) || source != "ESP32_JOB" ||
                !findUnsigned(line, "session_id", assignment.sessionId) ||
                assignment.sessionId == 0UL ||
                !findUnsigned(line, "run_id", assignment.runId) || assignment.runId == 0UL ||
                !findUnsigned(line, "motor_id", assignment.motorId) || assignment.motorId == 0UL ||
                !findString(line, "role", assignment.role) || !validRole(assignment.role))
            {
                file.close();
                return false;
            }
            previousAssignmentId = assignment.assignmentId;

            for (uint8_t index = 0U; index < pageCount; ++index)
            {
                if (selected[index].sessionId == assignment.sessionId)
                {
                    assignments[index] = assignment;
                    assignmentFound[index] = true;
                }
            }
        }
        file.close();
    }

    for (uint8_t index = 0U; index < pageCount; ++index)
    {
        JobRuntimeState state;
        JobSnapshot snapshot;
        const uint32_t sessionId = selected[index].sessionId;
        if (!JobStateStore::readPersisted(m_storage, sessionId, state) ||
            !JobSnapshotStore::readPersisted(m_storage, sessionId, snapshot) ||
            state.executionState != JobExecutionState::ProgramCompleted ||
            state.deliveryState != JobDeliveryState::Accepted ||
            state.jobId != snapshot.jobId || state.sessionId != snapshot.sessionId ||
            state.lastRunId == 0UL || state.completedRuns == 0U ||
            state.completedRuns != snapshot.repeatTarget ||
            !snapshot.linkage.isValid())
        {
            return false;
        }

        if (assignmentFound[index])
        {
            if (snapshot.linkage.linked ||
                assignments[index].runId != state.lastRunId)
                return false;
        }

        if (count > 0U) json += ',';
        json += F("{\"source\":\"ESP32_JOB\",\"session_id\":");
        json += state.sessionId;
        json += F(",\"run_id\":"); json += state.lastRunId;
        json += F(",\"job_id\":"); json += state.jobId;
        json += F(",\"status\":\"COMPLETED\",\"winding_type\":\"");
        json += snapshot.type == RemoteJobType::Starting ? F("STARTING") : F("WORKING");
        json += F("\",\"coil_count\":"); json += snapshot.coilCount;
        json += F(",\"program\":\""); json += snapshotProgram(snapshot);
        json += F("\",\"repeat_target\":"); json += snapshot.repeatTarget;
        json += F(",\"completed_runs\":"); json += state.completedRuns;
        json += F(",\"start_observed\":true,\"received_uptime_ms\":");
        json += state.updatedUptimeMs;
        json += F(",\"immutable_linkage\":");
        json += snapshot.linkage.linked ? F("true") : F("false");
        json += F(",\"repair_id\":");
        if (snapshot.linkage.linked) json += snapshot.linkage.repairId;
        else json += F("null");
        json += F(",\"assigned_motor_id\":");
        if (snapshot.linkage.linked) json += snapshot.linkage.motorId;
        else if (assignmentFound[index]) json += assignments[index].motorId;
        else json += F("null");
        json += F(",\"assignment_role\":");
        if (snapshot.linkage.linked)
        {
            json += '"';
            json += snapshot.type == RemoteJobType::Starting ? F("STARTING") : F("WORKING");
            json += '"';
        }
        else if (assignmentFound[index])
        {
            json += '"'; json += jsonEscape(assignments[index].role); json += '"';
        }
        else json += F("null");
        json += F(",\"assignment_id\":");
        if (assignmentFound[index]) json += assignments[index].assignmentId;
        else json += F("null");
        json += '}';
        ++count;
    }

    if (count > 0U)
        nextCursorSessionId = selected[count - 1U].sessionId;
    return true;
}

AutonomousWindingAssignResult
AutonomousWindingArchive::assignCompletedWebJobMotorChecked(
    uint32_t sessionId,
    uint32_t runId,
    uint32_t motorId,
    const String& role,
    uint32_t& assignmentId)
{
    assignmentId = 0UL;
    if (!ready() || sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        !validRole(role))
        return AutonomousWindingAssignResult::Invalid;

    JobRuntimeState state;
    JobSnapshot snapshot;
    if (!JobStateStore::readPersisted(m_storage, sessionId, state) ||
        !JobSnapshotStore::readPersisted(m_storage, sessionId, snapshot))
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;

    if (state.executionState != JobExecutionState::ProgramCompleted ||
        state.deliveryState != JobDeliveryState::Accepted ||
        state.jobId != snapshot.jobId || state.sessionId != snapshot.sessionId ||
        state.lastRunId != runId || state.completedRuns == 0U ||
        state.completedRuns != snapshot.repeatTarget ||
        !snapshot.linkage.isValid())
        return AutonomousWindingAssignResult::TaskNotFound;

    if (snapshot.linkage.linked)
        return AutonomousWindingAssignResult::Invalid;

    uint32_t highestId = 0UL;
    if (m_storage.exists(WebAssignmentsPath))
    {
        File existing = m_storage.open(WebAssignmentsPath, FILE_READ);
        if (!existing || existing.isDirectory())
        {
            if (existing) existing.close();
            return AutonomousWindingAssignResult::StorageUnavailable;
        }
        while (existing.available())
        {
            const String line = existing.readStringUntil('\n');
            if (line.length() == 0U) continue;
            uint32_t schema = 0UL;
            uint32_t currentId = 0UL;
            uint32_t currentSession = 0UL;
            uint32_t currentRun = 0UL;
            uint32_t currentMotor = 0UL;
            String source;
            String currentRole;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "schema_version", schema) || schema != 1UL ||
                !findUnsigned(line, "assignment_id", currentId) || currentId == 0UL ||
                currentId <= highestId ||
                !findString(line, "source", source) || source != "ESP32_JOB" ||
                !findUnsigned(line, "session_id", currentSession) || currentSession == 0UL ||
                !findUnsigned(line, "run_id", currentRun) || currentRun == 0UL ||
                !findUnsigned(line, "motor_id", currentMotor) || currentMotor == 0UL ||
                !findString(line, "role", currentRole) || !validRole(currentRole))
            {
                existing.close();
                return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
            }
            highestId = currentId;
        }
        existing.close();
    }
    if (highestId == 0xFFFFFFFFUL)
        return AutonomousWindingAssignResult::WriteFailed;
    assignmentId = highestId + 1UL;

    File file = m_storage.open(WebAssignmentsPath, FILE_APPEND);
    if (!file)
    {
        assignmentId = 0UL;
        return AutonomousWindingAssignResult::StorageUnavailable;
    }

    String record;
    record.reserve(240U);
    record = F("{\"schema_version\":1,\"assignment_id\":");
    record += assignmentId;
    record += F(",\"source\":\"ESP32_JOB\",\"session_id\":");
    record += sessionId;
    record += F(",\"run_id\":"); record += runId;
    record += F(",\"motor_id\":"); record += motorId;
    record += F(",\"role\":\""); record += role;
    record += F("\",\"assigned_uptime_ms\":"); record += millis();
    record += F("}\n");

    const size_t written = file.print(record);
    file.flush();
    file.close();
    if (written != record.length())
    {
        assignmentId = 0UL;
        return AutonomousWindingAssignResult::WriteFailed;
    }
    return AutonomousWindingAssignResult::Assigned;
}
}
