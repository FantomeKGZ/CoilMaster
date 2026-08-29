#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"

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

    // Keep the completed-task proof at the assignment mutation boundary. The
    // projection path performs an earlier preflight before canonical mutation,
    // but that proof must not replace this authoritative reread.
    bool taskFound = false;
    if (!completedTaskExists(sessionId, runId, taskFound))
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
    if (!taskFound)
        return AutonomousWindingAssignResult::TaskNotFound;

    // One mutation-time assignment-ledger scan supplies all three facts needed
    // before append: strict journal integrity/order, an exact retry/conflict for
    // this session+run, and the next assignment id. This closes the window where
    // an assignment could appear after canonical projection preflight while
    // retaining the authoritative reread immediately before the append.
    uint32_t highestId = 0UL;
    bool exactFound = false;
    if (m_storage.exists(AssignmentsPath))
    {
        File existing = m_storage.open(AssignmentsPath, FILE_READ);
        if (!existing || existing.isDirectory())
        {
            if (existing) existing.close();
            return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
        }

        while (existing.available())
        {
            const String line = existing.readStringUntil('\n');
            if (line.length() == 0U) continue;

            AutonomousWindingAssignment current;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseAssignment(line, current) || !current.isValid() ||
                current.assignmentId <= highestId)
            {
                existing.close();
                assignmentId = 0UL;
                return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
            }
            highestId = current.assignmentId;

            if (current.sessionId == sessionId && current.runId == runId)
            {
                if (exactFound)
                {
                    existing.close();
                    assignmentId = 0UL;
                    return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
                }
                exactFound = true;
                assignmentId = current.assignmentId;
                if (current.motorId != motorId || current.role != role)
                {
                    existing.close();
                    assignmentId = 0UL;
                    return AutonomousWindingAssignResult::Invalid;
                }
            }
        }
        existing.close();
    }

    if (exactFound)
        return AutonomousWindingAssignResult::Assigned;
    if (highestId == 0xFFFFFFFFUL)
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
    assignmentId = highestId + 1UL;

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