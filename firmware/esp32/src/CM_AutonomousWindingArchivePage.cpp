#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
constexpr uint8_t AutonomousWindingArchive::MaxTaskPageSize;

namespace
{
struct PageTask
{
    RemoteWindingEvent event;
    uint32_t receivedUptimeMs;
    bool startObserved;
    AutonomousWindingAssignment assignment;
    bool assigned;

    PageTask()
        : event(), receivedUptimeMs(0UL), startObserved(false),
          assignment(), assigned(false)
    {
    }
};

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
}

bool AutonomousWindingArchive::appendTasksPageJson(
    String& json,
    const String& programQuery,
    uint8_t tolerancePercent,
    uint32_t cursor,
    uint8_t limit,
    uint16_t& count,
    uint32_t& nextCursor,
    bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;

    if (!ready() || tolerancePercent > 50U ||
        limit == 0U || limit > MaxTaskPageSize)
    {
        return false;
    }
    if (programQuery.length() > 0U &&
        !WindingProgramParser::valid(programQuery))
    {
        return false;
    }

    if (!m_storage.exists(EventsPath))
        return cursor == 0UL;

    File file = m_storage.open(EventsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL || cursor > rawSize)
    {
        file.close();
        return false;
    }
    const uint32_t fileSize = static_cast<uint32_t>(rawSize);

    // Cursors are opaque byte offsets returned by this method. Fail closed if a
    // caller supplies an offset in the middle of an NDJSON record.
    if (cursor > 0UL)
    {
        if (!file.seek(cursor - 1UL) || file.read() != '\n')
        {
            file.close();
            return false;
        }
    }
    if (!file.seek(cursor))
    {
        file.close();
        return false;
    }

    PageTask tasks[MaxTaskPageSize];

    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        RemoteWindingEvent event;
        uint32_t startObservedValue = 0UL;
        uint32_t receivedUptimeMs = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !parseEventRecord(line, event) ||
            !findUnsigned(line, "start_observed", startObservedValue) ||
            startObservedValue > 1UL ||
            !findUnsigned(line, "received_uptime_ms", receivedUptimeMs))
        {
            file.close();
            return false;
        }

        PageTask task;
        task.event = event;
        task.startObserved = startObservedValue == 1UL;
        task.receivedUptimeMs = receivedUptimeMs;

        if (event.type == RemoteEventType::RunCompleted)
        {
            // A completion claiming START was observed is only valid when this
            // reader consumed the matching START immediately before it. Seeing
            // it at a page boundary means the supplied cursor split one task.
            if (task.startObserved)
            {
                file.close();
                return false;
            }
        }
        else if (event.type == RemoteEventType::RunStarted)
        {
            if (!task.startObserved)
            {
                file.close();
                return false;
            }

            // Peek at the next non-empty record. A normal completed run is stored
            // as START then COMPLETE. Consume both as one logical API item. If the
            // next run is different, rewind to its record boundary so it becomes
            // the first record of the next logical task.
            bool nextFound = false;
            uint32_t nextRecordStart = 0UL;
            String nextLine;
            while (file.available())
            {
                nextRecordStart = static_cast<uint32_t>(file.position());
                nextLine = file.readStringUntil('\n');
                if (nextLine.length() > 0U)
                {
                    nextFound = true;
                    break;
                }
            }

            if (nextFound)
            {
                RemoteWindingEvent nextEvent;
                uint32_t nextStartObserved = 0UL;
                uint32_t nextReceivedUptimeMs = 0UL;
                if (!FlatJsonObjectValidator::valid(nextLine) ||
                    !parseEventRecord(nextLine, nextEvent) ||
                    !findUnsigned(nextLine, "start_observed", nextStartObserved) ||
                    nextStartObserved > 1UL ||
                    !findUnsigned(nextLine, "received_uptime_ms",
                                  nextReceivedUptimeMs))
                {
                    file.close();
                    return false;
                }

                if (nextEvent.runId < event.runId)
                {
                    file.close();
                    return false;
                }

                if (nextEvent.runId == event.runId)
                {
                    const bool validPair =
                        nextEvent.type == RemoteEventType::RunCompleted &&
                        nextEvent.sessionId == event.sessionId &&
                        nextStartObserved == 1UL &&
                        sameProgram(event, nextEvent);
                    if (!validPair)
                    {
                        file.close();
                        return false;
                    }

                    task.event = nextEvent;
                    task.startObserved = true;
                    task.receivedUptimeMs = nextReceivedUptimeMs;
                }
                else if (!file.seek(nextRecordStart))
                {
                    file.close();
                    return false;
                }
            }
        }
        else
        {
            file.close();
            return false;
        }

        const String program = programText(task.event);
        if (programQuery.length() > 0U &&
            !programMatches(program, programQuery, tolerancePercent))
        {
            continue;
        }

        tasks[count] = task;
        ++count;
    }

    const uint32_t pageEnd = static_cast<uint32_t>(file.position());
    hasMore = pageEnd < fileSize;
    nextCursor = hasMore ? pageEnd : 0UL;
    file.close();

    // Resolve assignments in one bounded pass instead of reopening the complete
    // assignments file once for every item in the page. Later assignment_id wins,
    // matching the existing latestAssignment() semantics.
    if (count > 0U && m_storage.exists(AssignmentsPath))
    {
        File assignments = m_storage.open(AssignmentsPath, FILE_READ);
        if (!assignments || assignments.isDirectory())
        {
            if (assignments) assignments.close();
            return false;
        }

        uint32_t previousAssignmentId = 0UL;
        while (assignments.available())
        {
            const String line = assignments.readStringUntil('\n');
            if (line.length() == 0U) continue;

            AutonomousWindingAssignment assignment;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseAssignment(line, assignment) ||
                assignment.assignmentId <= previousAssignmentId)
            {
                assignments.close();
                return false;
            }
            previousAssignmentId = assignment.assignmentId;

            for (uint16_t index = 0U; index < count; ++index)
            {
                PageTask& task = tasks[index];
                if (task.event.type == RemoteEventType::RunCompleted &&
                    task.event.sessionId == assignment.sessionId &&
                    task.event.runId == assignment.runId)
                {
                    task.assignment = assignment;
                    task.assigned = true;
                }
            }
        }
        assignments.close();
    }

    for (uint16_t index = 0U; index < count; ++index)
    {
        const PageTask& task = tasks[index];
        if (index > 0U) json += ',';

        json += F("{\"session_id\":");
        json += task.event.sessionId;
        json += F(",\"run_id\":");
        json += task.event.runId;
        json += F(",\"status\":\"");
        json += task.event.type == RemoteEventType::RunCompleted
            ? F("COMPLETED") : F("STARTED_NOT_COMPLETED");
        json += F("\",\"winding_type\":\"");
        json += windingTypeName(task.event.jobType);
        json += F("\",\"coil_count\":");
        json += task.event.coilCount;
        json += F(",\"program\":\"");
        json += programText(task.event);
        json += F("\",\"completed_runs\":");
        json += task.event.completedRuns;
        json += F(",\"start_observed\":");
        json += task.startObserved ? F("true") : F("false");
        json += F(",\"received_uptime_ms\":");
        json += task.receivedUptimeMs;
        json += F(",\"assigned_motor_id\":");
        if (task.assigned) json += task.assignment.motorId;
        else json += F("null");
        json += F(",\"assignment_role\":");
        if (task.assigned)
        {
            json += '"';
            json += jsonEscape(task.assignment.role);
            json += '"';
        }
        else json += F("null");
        json += F(",\"assignment_id\":");
        if (task.assigned) json += task.assignment.assignmentId;
        else json += F("null");
        json += '}';
    }

    return true;
}
}
