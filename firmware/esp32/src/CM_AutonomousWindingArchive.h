#ifndef CM_AUTONOMOUS_WINDING_ARCHIVE_H
#define CM_AUTONOMOUS_WINDING_ARCHIVE_H

#include <Arduino.h>
#include <FS.h>

#include "CM_UartEventReceiver.h"

namespace CM
{
enum class AutonomousWindingSaveResult : uint8_t
{
    Saved = 0U,
    Duplicate,
    Invalid,
    StorageUnavailable,
    WriteFailed
};

enum class AutonomousWindingAssignResult : uint8_t
{
    Assigned = 0U,
    Invalid,
    TaskNotFound,
    ArchiveIntegrityFailed,
    StorageUnavailable,
    WriteFailed
};

struct AutonomousWindingAssignment
{
    uint32_t assignmentId;
    uint32_t sessionId;
    uint32_t runId;
    uint32_t motorId;
    String role;

    AutonomousWindingAssignment()
        : assignmentId(0UL), sessionId(0UL), runId(0UL), motorId(0UL), role() {}

    bool isValid() const
    {
        return assignmentId != 0UL && sessionId != 0UL &&
               runId != 0UL && motorId != 0UL && role.length() > 0U;
    }
};

struct AutonomousWindingIntegrityMetrics
{
    uint32_t eventRecordCount;
    uint32_t startedRecordCount;
    uint32_t completedRecordCount;
    uint32_t assignmentRecordCount;

    AutonomousWindingIntegrityMetrics()
        : eventRecordCount(0UL), startedRecordCount(0UL),
          completedRecordCount(0UL), assignmentRecordCount(0UL)
    {
    }
};

class AutonomousWindingArchive
{
public:
    static constexpr uint8_t MaxTaskPageSize = 32U;

    explicit AutonomousWindingArchive(fs::FS& storage);

    bool begin();
    bool ready() const;

    AutonomousWindingSaveResult save(const RemoteWindingEvent& event);

    // Cursor is a byte offset into append-only events.ndjson and must point to a
    // logical task boundary. The returned cursor always starts the next task and
    // never splits RUN_STARTED/RUN_COMPLETED for one run.
    bool appendTasksPageJson(String& json,
                             const String& programQuery,
                             uint8_t tolerancePercent,
                             uint32_t cursor,
                             uint8_t limit,
                             uint16_t& count,
                             uint32_t& nextCursor,
                             bool& hasMore) const;

    bool assignMotor(uint32_t sessionId,
                     uint32_t runId,
                     uint32_t motorId,
                     const String& role,
                     uint32_t& assignmentId);

    // Performs the completed-task lookup and append in one archive operation so
    // HTTP callers do not scan events.ndjson once before assignMotor() and then
    // a second time inside it. Result preserves not-found vs integrity semantics.
    AutonomousWindingAssignResult assignMotorChecked(uint32_t sessionId,
                                                      uint32_t runId,
                                                      uint32_t motorId,
                                                      const String& role,
                                                      uint32_t& assignmentId);

    // Read-only authoritative audit used both at boot and by backup/deep-integrity
    // checks. It never creates directories or mutates persisted archive data.
    static bool validateStorage(fs::FS& storage,
                                AutonomousWindingIntegrityMetrics& metrics);

private:
    static constexpr const char* DirectoryPath = "/data/autonomous-windings";
    static constexpr const char* EventsPath = "/data/autonomous-windings/events.ndjson";
    static constexpr const char* AssignmentsPath = "/data/autonomous-windings/assignments.ndjson";

    bool ensureDirectories();
    bool completedTaskExists(uint32_t sessionId,
                             uint32_t runId,
                             bool& found) const;
    bool loadLastEvent(RemoteWindingEvent& event, bool& found) const;
    bool findEventReplay(const RemoteWindingEvent& event,
                         bool& exactMatch,
                         bool& conflict) const;
    bool matchingStartExists(const RemoteWindingEvent& event, bool& found) const;
    bool nextAssignmentId(uint32_t& assignmentId) const;

    static bool parseEventRecord(const String& line,
                                 RemoteWindingEvent& event);
    static bool parseAssignment(const String& line,
                                AutonomousWindingAssignment& assignment);
    static bool programMatches(const String& candidate,
                               const String& query,
                               uint8_t tolerancePercent);
    static String programText(const RemoteWindingEvent& event);
    static bool validRole(const String& role);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);
    static const char* eventName(RemoteEventType type);
    static const char* windingTypeName(RemoteJobType type);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_AUTONOMOUS_WINDING_ARCHIVE_H
