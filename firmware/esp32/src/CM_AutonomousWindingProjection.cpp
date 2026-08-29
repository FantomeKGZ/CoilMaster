#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"

namespace CM
{
namespace
{
String webSnapshotProgram(const JobSnapshot& snapshot)
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

String autonomousCreatedAt(uint32_t sessionId, uint32_t runId)
{
    // Autonomous records currently persist monotonic uptime rather than a
    // trusted wall-clock timestamp. Keep created_at deterministic and honest
    // instead of fabricating a calendar time; provenance fields carry the
    // authoritative retry identity.
    String value = F("AUTONOMOUS-");
    value += sessionId;
    value += '-';
    value += runId;
    return value;
}
}

AutonomousWindingAssignResult AutonomousWindingArchive::ensureCanonicalProjection(
    uint32_t sessionId,
    uint32_t runId,
    uint32_t motorId,
    const String& role,
    const String& program,
    uint16_t repeatTarget,
    bool replaceExisting)
{
    if (sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        (role != "WORKING" && role != "STARTING") ||
        program.length() == 0U || repeatTarget == 0U)
    {
        return AutonomousWindingAssignResult::Invalid;
    }

    if (!m_motorWindingVersions.ready() && !m_motorWindingVersions.begin())
        return AutonomousWindingAssignResult::ProjectionFailed;

    uint32_t projectedMotorId = 0UL;
    uint32_t projectedVersionId = 0UL;
    bool projectionFound = false;
    if (!m_motorWindingVersions.findAutonomousProjection(sessionId,
                                                          runId,
                                                          role,
                                                          projectedMotorId,
                                                          projectedVersionId,
                                                          projectionFound))
    {
        return AutonomousWindingAssignResult::ProjectionFailed;
    }
    if (projectionFound)
    {
        return projectedMotorId == motorId
            ? AutonomousWindingAssignResult::Assigned
            : AutonomousWindingAssignResult::Invalid;
    }

    NewMotorWindingVersion latest;
    uint32_t latestVersionId = 0UL;
    bool latestFound = false;
    if (!m_motorWindingVersions.loadLatestByMotor(motorId,
                                                   latest,
                                                   latestVersionId,
                                                   latestFound))
    {
        return AutonomousWindingAssignResult::ProjectionFailed;
    }

    if (!latestFound && role == "STARTING")
        return AutonomousWindingAssignResult::StartingRequiresWorking;

    if (latestFound)
    {
        const bool targetOccupied = role == "WORKING"
            ? latest.working.present : latest.starting.present;
        if (targetOccupied && !replaceExisting)
            return AutonomousWindingAssignResult::RoleOccupied;
    }

    NewMotorWindingVersion next;
    if (latestFound)
    {
        // Preserve the complete untargeted role, including conductors and pitch.
        next.working = latest.working;
        next.starting = latest.starting;
        next.previousVersionId = latestVersionId;
    }
    next.motorId = motorId;
    next.sourceAutonomousSessionId = sessionId;
    next.sourceAutonomousRunId = runId;
    next.sourceAutonomousRole = role;
    next.versionKind = F("AUTONOMOUS_ASSIGNMENT");
    next.createdAt = autonomousCreatedAt(sessionId, runId);
    next.comment = F("Projected from completed autonomous winding assignment");

    MotorWindingRoleSpec incoming;
    incoming.present = true;
    incoming.coilProgram = program;
    incoming.repeatTarget = repeatTarget;
    incoming.coilPitch = 0U;
    incoming.conductorCount = 0U;
    if (role == "WORKING") next.working = incoming;
    else next.starting = incoming;

    uint32_t appendedVersionId = 0UL;
    if (!m_motorWindingVersions.append(next, appendedVersionId) ||
        appendedVersionId == 0UL)
    {
        return AutonomousWindingAssignResult::ProjectionFailed;
    }
    return AutonomousWindingAssignResult::Assigned;
}

AutonomousWindingAssignResult AutonomousWindingArchive::assignMotorChecked(
    uint32_t sessionId,
    uint32_t runId,
    uint32_t motorId,
    const String& role,
    bool replaceExisting,
    uint32_t& assignmentId)
{
    assignmentId = 0UL;
    if (!ready()) return AutonomousWindingAssignResult::StorageUnavailable;
    if (sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        (role != "WORKING" && role != "STARTING"))
    {
        return AutonomousWindingAssignResult::Invalid;
    }

    RemoteWindingEvent completed;
    bool completedFound = false;
    if (m_storage.exists(EventsPath))
    {
        File events = m_storage.open(EventsPath, FILE_READ);
        if (!events || events.isDirectory())
        {
            if (events) events.close();
            return AutonomousWindingAssignResult::StorageUnavailable;
        }
        while (events.available())
        {
            const String line = events.readStringUntil('\n');
            if (line.length() == 0U) continue;
            RemoteWindingEvent event;
            if (!FlatJsonObjectValidator::valid(line) || !parseEventRecord(line, event))
            {
                events.close();
                return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
            }
            if (event.sessionId == sessionId && event.runId == runId &&
                event.type == RemoteEventType::RunCompleted)
            {
                if (completedFound)
                {
                    events.close();
                    return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
                }
                completed = event;
                completedFound = true;
            }
        }
        events.close();
    }
    if (!completedFound) return AutonomousWindingAssignResult::TaskNotFound;

    // Preflight historical linkage before creating a canonical version. This
    // prevents a conflicting retry from projecting to a different motor/role.
    if (m_storage.exists(AssignmentsPath))
    {
        File assignments = m_storage.open(AssignmentsPath, FILE_READ);
        if (!assignments || assignments.isDirectory())
        {
            if (assignments) assignments.close();
            return AutonomousWindingAssignResult::StorageUnavailable;
        }
        uint32_t previousId = 0UL;
        bool exactFound = false;
        while (assignments.available())
        {
            const String line = assignments.readStringUntil('\n');
            if (line.length() == 0U) continue;
            AutonomousWindingAssignment current;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseAssignment(line, current) || !current.isValid() ||
                current.assignmentId <= previousId)
            {
                assignments.close();
                return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
            }
            previousId = current.assignmentId;
            if (current.sessionId == sessionId && current.runId == runId)
            {
                if (exactFound || current.motorId != motorId || current.role != role)
                {
                    assignments.close();
                    return exactFound
                        ? AutonomousWindingAssignResult::ArchiveIntegrityFailed
                        : AutonomousWindingAssignResult::Invalid;
                }
                exactFound = true;
                assignmentId = current.assignmentId;
            }
        }
        assignments.close();
    }

    const AutonomousWindingAssignResult projected = ensureCanonicalProjection(
        sessionId, runId, motorId, role, programText(completed),
        completed.completedRuns, replaceExisting);
    if (projected != AutonomousWindingAssignResult::Assigned) return projected;

    if (assignmentId != 0UL) return AutonomousWindingAssignResult::Assigned;
    return assignMotorChecked(sessionId, runId, motorId, role, assignmentId);
}

AutonomousWindingAssignResult
AutonomousWindingArchive::assignCompletedWebJobMotorChecked(
    uint32_t sessionId,
    uint32_t runId,
    uint32_t motorId,
    const String& role,
    bool replaceExisting,
    uint32_t& assignmentId)
{
    assignmentId = 0UL;
    if (!ready()) return AutonomousWindingAssignResult::StorageUnavailable;
    if (sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        (role != "WORKING" && role != "STARTING"))
    {
        return AutonomousWindingAssignResult::Invalid;
    }

    JobRuntimeState state;
    JobSnapshot snapshot;
    if (!JobStateStore::readPersisted(m_storage, sessionId, state) ||
        !JobSnapshotStore::readPersisted(m_storage, sessionId, snapshot))
    {
        return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
    }
    if (state.executionState != JobExecutionState::ProgramCompleted ||
        state.deliveryState != JobDeliveryState::Accepted ||
        state.jobId != snapshot.jobId || state.sessionId != snapshot.sessionId ||
        state.lastRunId != runId || state.completedRuns == 0U ||
        state.completedRuns != snapshot.repeatTarget ||
        !snapshot.linkage.isValid())
    {
        return AutonomousWindingAssignResult::TaskNotFound;
    }
    if (snapshot.linkage.linked) return AutonomousWindingAssignResult::Invalid;

    // Validate existing assignment linkage before canonical mutation.
    if (m_storage.exists(WebAssignmentsPath))
    {
        File assignments = m_storage.open(WebAssignmentsPath, FILE_READ);
        if (!assignments || assignments.isDirectory())
        {
            if (assignments) assignments.close();
            return AutonomousWindingAssignResult::StorageUnavailable;
        }
        uint32_t previousId = 0UL;
        bool exactFound = false;
        while (assignments.available())
        {
            const String line = assignments.readStringUntil('\n');
            if (line.length() == 0U) continue;
            uint32_t schema = 0UL, currentId = 0UL, currentSession = 0UL;
            uint32_t currentRun = 0UL, currentMotor = 0UL;
            String source, currentRole;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "schema_version", schema) || schema != 1UL ||
                !findUnsigned(line, "assignment_id", currentId) || currentId == 0UL ||
                currentId <= previousId ||
                !findString(line, "source", source) || source != "ESP32_JOB" ||
                !findUnsigned(line, "session_id", currentSession) || currentSession == 0UL ||
                !findUnsigned(line, "run_id", currentRun) || currentRun == 0UL ||
                !findUnsigned(line, "motor_id", currentMotor) || currentMotor == 0UL ||
                !findString(line, "role", currentRole) || !validRole(currentRole))
            {
                assignments.close();
                return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
            }
            previousId = currentId;
            if (currentSession == sessionId && currentRun == runId)
            {
                if (exactFound)
                {
                    assignments.close();
                    return AutonomousWindingAssignResult::ArchiveIntegrityFailed;
                }
                if (currentMotor != motorId || currentRole != role)
                {
                    assignments.close();
                    return AutonomousWindingAssignResult::Invalid;
                }
                exactFound = true;
                assignmentId = currentId;
            }
        }
        assignments.close();
    }

    const AutonomousWindingAssignResult projected = ensureCanonicalProjection(
        sessionId, runId, motorId, role, webSnapshotProgram(snapshot),
        snapshot.repeatTarget, replaceExisting);
    if (projected != AutonomousWindingAssignResult::Assigned) return projected;

    if (assignmentId != 0UL) return AutonomousWindingAssignResult::Assigned;
    return assignCompletedWebJobMotorChecked(sessionId, runId, motorId, role,
                                             assignmentId);
}
}
