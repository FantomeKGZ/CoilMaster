#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool incrementCount(uint32_t& value)
{
    if (value == 0xFFFFFFFFUL) return false;
    ++value;
    return true;
}
}

bool AutonomousWindingArchive::validateStorage(
    fs::FS& storage,
    AutonomousWindingIntegrityMetrics& metrics)
{
    metrics = AutonomousWindingIntegrityMetrics();

    // Use the archive's production validators without begin(): backup audit is
    // strictly read-only and must never create a missing directory as a side
    // effect of checking snapshot stability.
    AutonomousWindingArchive archive(storage);
    if (!archive.validateEvents() || !archive.validateAssignments())
        return false;

    if (storage.exists(EventsPath))
    {
        File file = storage.open(EventsPath, FILE_READ);
        if (!file || file.isDirectory())
        {
            if (file) file.close();
            return false;
        }

        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;

            RemoteWindingEvent event;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseEventRecord(line, event) ||
                !incrementCount(metrics.eventRecordCount))
            {
                file.close();
                return false;
            }

            if (event.type == RemoteEventType::RunStarted)
            {
                if (!incrementCount(metrics.startedRecordCount))
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
        }
        file.close();
    }

    if (storage.exists(AssignmentsPath))
    {
        File file = storage.open(AssignmentsPath, FILE_READ);
        if (!file || file.isDirectory())
        {
            if (file) file.close();
            return false;
        }

        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;

            AutonomousWindingAssignment assignment;
            if (!FlatJsonObjectValidator::valid(line) ||
                !parseAssignment(line, assignment) ||
                !incrementCount(metrics.assignmentRecordCount))
            {
                file.close();
                return false;
            }
        }
        file.close();
    }

    return true;
}
}
