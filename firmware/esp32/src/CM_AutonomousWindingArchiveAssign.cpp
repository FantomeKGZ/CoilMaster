#include "CM_AutonomousWindingArchive.h"

namespace CM
{
AutonomousWindingAssignResult AutonomousWindingArchive::assignMotorChecked(
    uint32_t sessionId,
    uint32_t runId,
    uint32_t motorId,
    const String& role,
    uint32_t& assignmentId)
{
    assignmentId = 0UL;

    if (!ready())
        return AutonomousWindingAssignResult::StorageUnavailable;
    if (sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        !validRole(role))
    {
        return AutonomousWindingAssignResult::Invalid;
    }

    // One authoritative completed-task scan. The HTTP layer must not perform the
    // same full events.ndjson lookup before entering this storage boundary.
    bool taskFound = false;
    if (!completedTaskExists(sessionId, runId, taskFound))
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
    if (!taskFound)
        return AutonomousWindingAssignResult::TaskNotFound;

    // Keep the existing full assignment-ledger validation when deriving the next
    // id. Unlike LOCAL_EVT append this is an occasional operator action, and the
    // scan protects against runtime corruption/duplicate assignment ids until
    // real populated-dataset metrics justify a separate persisted high-water.
    if (!nextAssignmentId(assignmentId))
    {
        assignmentId = 0UL;
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
    }

    File file = m_storage.open(AssignmentsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        assignmentId = 0UL;
        return AutonomousWindingAssignResult::StorageUnavailable;
    }

    String record;
    record.reserve(220U);
    record = F("{\"schema_version\":1,\"assignment_id\":");
    record += assignmentId;
    record += F(",\"session_id\":");
    record += sessionId;
    record += F(",\"run_id\":");
    record += runId;
    record += F(",\"motor_id\":");
    record += motorId;
    record += F(",\"role\":\"");
    record += role;
    record += F("\",\"assigned_uptime_ms\":");
    record += millis();
    record += F("}\n");

    const size_t written = file.print(record);
    file.flush();
    file.close();
    if (written != record.length())
    {
        m_ready = false;
        assignmentId = 0UL;
        return AutonomousWindingAssignResult::WriteFailed;
    }

    return AutonomousWindingAssignResult::Assigned;
}
}
