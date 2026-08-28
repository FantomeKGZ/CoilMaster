#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"

namespace CM
{
namespace
{
constexpr uint8_t AssignmentAuditBatchSize = 32U;

struct AssignmentReference
{
    uint32_t sessionId;
    uint32_t runId;
    bool found;

    AssignmentReference() : sessionId(0UL), runId(0UL), found(false) {}
};

bool incrementCount(uint32_t& value)
{
    if (value == 0xFFFFFFFFUL) return false;
    ++value;
    return true;
}

bool sameProgram(const RemoteWindingEvent& left,
                 const RemoteWindingEvent& right)
{
    if (left.jobType != right.jobType || left.coilCount != right.coilCount)
        return false;
    for (uint8_t index = 0U; index < left.coilCount; ++index)
    {
        if (left.turns[index] != right.turns[index]) return false;
    }
    return true;
}

bool requireNdjsonTermination(File& file)
{
    const size_t size = file.size();
    if (size == 0U) return true;
    if (!file.seek(size - 1U) || file.read() != '\n') return false;
    return file.seek(0U);
}
}

bool AutonomousWindingArchive::validateStorage(
    fs::FS& storage,
    AutonomousWindingIntegrityMetrics& metrics)
{
    metrics = AutonomousWindingIntegrityMetrics();

    const auto validateAssignmentBatch =
        [&](AssignmentReference* references, uint8_t count) -> bool
    {
        if (count == 0U) return true;
        if (!storage.exists(EventsPath)) return false;

        File events = storage.open(EventsPath, FILE_READ);
        if (!events || events.isDirectory())
        {
            if (events) events.close();
            return false;
        }

        uint8_t unresolved = count;
        while (events.available() && unresolved > 0U)
        {
            const String line = events.readStringUntil('\n');
            if (line.length() == 0U) continue;

            RemoteWindingEvent event;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseEventRecord(line, event))
            {
                events.close();
                return false;
            }
            if (event.type != RemoteEventType::RunCompleted) continue;

            for (uint8_t index = 0U; index < count; ++index)
            {
                AssignmentReference& reference = references[index];
                if (!reference.found &&
                    reference.sessionId == event.sessionId &&
                    reference.runId == event.runId)
                {
                    reference.found = true;
                    --unresolved;
                }
            }
        }
        events.close();
        return unresolved == 0U;
    };

    if (storage.exists(EventsPath))
    {
        File file = storage.open(EventsPath, FILE_READ);
        if (!file || file.isDirectory())
        {
            if (file) file.close();
            return false;
        }
        if (!requireNdjsonTermination(file))
        {
            file.close();
            return false;
        }

        bool havePrevious = false;
        uint32_t previousRunId = 0UL;
        uint32_t previousSessionId = 0UL;
        bool pendingStart = false;
        RemoteWindingEvent startEvent;

        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;

            RemoteWindingEvent event;
            uint32_t startObserved = 0UL;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseEventRecord(line, event) ||
                !findUnsigned(line, "start_observed", startObserved) ||
                startObserved > 1UL ||
                !incrementCount(metrics.eventRecordCount))
            {
                file.close();
                return false;
            }

            if (event.type == RemoteEventType::RunStarted)
            {
                if (startObserved != 1UL ||
                    !incrementCount(metrics.startedRecordCount))
                {
                    file.close();
                    return false;
                }
            }
            else if (event.type == RemoteEventType::RunCompleted)
            {
                if (!incrementCount(metrics.completedRecordCount))
                {
                    file.close();
                    return false;
                }
            }
            else
            {
                file.close();
                return false;
            }

            if (havePrevious)
            {
                if (event.runId < previousRunId ||
                    (event.runId > previousRunId &&
                     event.sessionId < previousSessionId))
                {
                    file.close();
                    return false;
                }

                if (event.runId == previousRunId)
                {
                    const bool validPair =
                        pendingStart &&
                        event.type == RemoteEventType::RunCompleted &&
                        startObserved == 1UL &&
                        event.sessionId == previousSessionId &&
                        event.sessionId == startEvent.sessionId &&
                        sameProgram(startEvent, event);
                    if (!validPair)
                    {
                        file.close();
                        return false;
                    }
                    pendingStart = false;
                }
                else
                {
                    pendingStart = false;
                    if (event.type == RemoteEventType::RunCompleted &&
                        startObserved == 1UL)
                    {
                        file.close();
                        return false;
                    }
                }
            }
            else if (event.type == RemoteEventType::RunCompleted &&
                     startObserved == 1UL)
            {
                file.close();
                return false;
            }

            if (event.type == RemoteEventType::RunStarted)
            {
                pendingStart = true;
                startEvent = event;
            }

            havePrevious = true;
            previousRunId = event.runId;
            previousSessionId = event.sessionId;
        }
        file.close();
    }

    if (storage.exists(AssignmentsPath))
    {
        File assignments = storage.open(AssignmentsPath, FILE_READ);
        if (!assignments || assignments.isDirectory())
        {
            if (assignments) assignments.close();
            return false;
        }
        if (!requireNdjsonTermination(assignments))
        {
            assignments.close();
            return false;
        }

        uint32_t previousAssignmentId = 0UL;
        AssignmentReference references[AssignmentAuditBatchSize];
        uint8_t batchCount = 0U;

        while (assignments.available())
        {
            const String line = assignments.readStringUntil('\n');
            if (line.length() == 0U) continue;

            AutonomousWindingAssignment assignment;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseAssignment(line, assignment) ||
                assignment.assignmentId <= previousAssignmentId ||
                !incrementCount(metrics.assignmentRecordCount))
            {
                assignments.close();
                return false;
            }
            previousAssignmentId = assignment.assignmentId;

            AssignmentReference& reference = references[batchCount++];
            reference.sessionId = assignment.sessionId;
            reference.runId = assignment.runId;
            reference.found = false;

            if (batchCount == AssignmentAuditBatchSize)
            {
                if (!validateAssignmentBatch(references, batchCount))
                {
                    assignments.close();
                    return false;
                }
                batchCount = 0U;
            }
        }

        if (batchCount > 0U &&
            !validateAssignmentBatch(references, batchCount))
        {
            assignments.close();
            return false;
        }
        assignments.close();
    }

    // ESP32_JOB linkage is a separate append-only journal. Each row must point
    // to the exact terminal persisted job state and immutable snapshot. A job
    // that already had immutable repair/motor linkage cannot receive this later
    // manual assignment path.
    if (storage.exists(WebAssignmentsPath))
    {
        File assignments = storage.open(WebAssignmentsPath, FILE_READ);
        if (!assignments || assignments.isDirectory())
        {
            if (assignments) assignments.close();
            return false;
        }
        if (!requireNdjsonTermination(assignments))
        {
            assignments.close();
            return false;
        }

        uint32_t previousAssignmentId = 0UL;
        while (assignments.available())
        {
            const String line = assignments.readStringUntil('\n');
            if (line.length() == 0U) continue;

            uint32_t schema = 0UL;
            uint32_t assignmentId = 0UL;
            uint32_t sessionId = 0UL;
            uint32_t runId = 0UL;
            uint32_t motorId = 0UL;
            uint32_t assignedUptimeMs = 0UL;
            String source;
            String role;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "schema_version", schema) || schema != 1UL ||
                !findUnsigned(line, "assignment_id", assignmentId) ||
                assignmentId == 0UL || assignmentId <= previousAssignmentId ||
                !findString(line, "source", source) || source != "ESP32_JOB" ||
                !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
                !findUnsigned(line, "run_id", runId) || runId == 0UL ||
                !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
                !findString(line, "role", role) || !validRole(role) ||
                !findUnsigned(line, "assigned_uptime_ms", assignedUptimeMs) ||
                !incrementCount(metrics.webAssignmentRecordCount))
            {
                assignments.close();
                return false;
            }
            previousAssignmentId = assignmentId;

            JobRuntimeState state;
            JobSnapshot snapshot;
            if (!JobStateStore::readPersisted(storage, sessionId, state) ||
                !JobSnapshotStore::readPersisted(storage, sessionId, snapshot) ||
                state.executionState != JobExecutionState::ProgramCompleted ||
                state.deliveryState != JobDeliveryState::Accepted ||
                state.jobId != snapshot.jobId ||
                state.sessionId != snapshot.sessionId ||
                state.sessionId != sessionId ||
                state.lastRunId != runId ||
                state.completedRuns == 0U ||
                state.completedRuns != snapshot.repeatTarget ||
                !snapshot.linkage.isValid() || snapshot.linkage.linked)
            {
                assignments.close();
                return false;
            }
        }
        assignments.close();
    }

    return true;
}
}
