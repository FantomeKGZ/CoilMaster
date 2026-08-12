#include "CM_AutonomousWindingArchive.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
AutonomousWindingArchive::AutonomousWindingArchive(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool AutonomousWindingArchive::begin()
{
    m_ready = false;
    if (!ensureDirectories()) return false;

    AutonomousWindingIntegrityMetrics metrics;
    if (!validateStorage(m_storage, metrics)) return false;

    m_ready = true;
    return true;
}

bool AutonomousWindingArchive::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open(DirectoryPath, FILE_READ);
    if (!directory) return false;
    const bool result = directory.isDirectory();
    directory.close();
    return result;
}

AutonomousWindingSaveResult AutonomousWindingArchive::save(
    const RemoteWindingEvent& event)
{
    if (!ready()) return AutonomousWindingSaveResult::StorageUnavailable;
    if (!event.localStandalone || !event.hasProgram() ||
        event.sessionId == 0UL || event.runId == 0UL ||
        (event.type != RemoteEventType::RunStarted &&
         event.type != RemoteEventType::RunCompleted) ||
        (event.type == RemoteEventType::RunStarted && event.completedRuns != 0U) ||
        (event.type == RemoteEventType::RunCompleted && event.completedRuns == 0U))
    {
        return AutonomousWindingSaveResult::Invalid;
    }

    bool duplicate = false;
    bool replayConflict = false;
    if (!findEventReplay(event, duplicate, replayConflict) || replayConflict)
        return AutonomousWindingSaveResult::Invalid;
    if (duplicate) return AutonomousWindingSaveResult::Duplicate;

    bool startObserved = event.type == RemoteEventType::RunStarted;
    if (event.type == RemoteEventType::RunCompleted)
    {
        if (!matchingStartExists(event, startObserved))
            return AutonomousWindingSaveResult::Invalid;
    }

    File file = m_storage.open(EventsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return AutonomousWindingSaveResult::StorageUnavailable;
    }

    String record;
    record.reserve(320U);
    record = F("{\"schema_version\":1,\"source\":\"ARDUINO_LOCAL\",\"session_id\":");
    record += event.sessionId;
    record += F(",\"run_id\":");
    record += event.runId;
    record += F(",\"event\":\"");
    record += eventName(event.type);
    record += F("\",\"winding_type\":\"");
    record += windingTypeName(event.jobType);
    record += F("\",\"coil_count\":");
    record += event.coilCount;
    record += F(",\"program\":\"");
    record += programText(event);
    record += F("\",\"completed_runs\":");
    record += event.completedRuns;
    record += F(",\"start_observed\":");
    record += startObserved ? 1 : 0;
    record += F(",\"received_uptime_ms\":");
    record += millis();
    record += F("}\n");

    const size_t written = file.print(record);
    file.flush();
    file.close();
    if (written != record.length())
    {
        m_ready = false;
        return AutonomousWindingSaveResult::WriteFailed;
    }
    return AutonomousWindingSaveResult::Saved;
}

bool AutonomousWindingArchive::completedTaskExists(uint32_t sessionId,
                                                    uint32_t runId,
                                                    bool& found) const
{
    found = false;
    if (sessionId == 0UL || runId == 0UL) return false;
    if (!m_storage.exists(EventsPath)) return true;

    File file = m_storage.open(EventsPath, FILE_READ);
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
            !parseEventRecord(line, event))
        {
            file.close();
            return false;
        }
        if (event.sessionId == sessionId && event.runId == runId &&
            event.type == RemoteEventType::RunCompleted)
        {
            found = true;
            file.close();
            return true;
        }
    }
    file.close();
    return true;
}

bool AutonomousWindingArchive::assignMotor(uint32_t sessionId,
                                           uint32_t runId,
                                           uint32_t motorId,
                                           const String& role,
                                           uint32_t& assignmentId)
{
    assignmentId = 0UL;
    if (!ready() || sessionId == 0UL || runId == 0UL || motorId == 0UL ||
        !validRole(role))
    {
        return false;
    }

    bool taskFound = false;
    if (!completedTaskExists(sessionId, runId, taskFound) || !taskFound ||
        !nextAssignmentId(assignmentId))
    {
        return false;
    }

    File file = m_storage.open(AssignmentsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
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
        return false;
    }
    return true;
}

bool AutonomousWindingArchive::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists(DirectoryPath) && !m_storage.mkdir(DirectoryPath))
        return false;
    return true;
}

bool AutonomousWindingArchive::loadLastEvent(RemoteWindingEvent& event,
                                             bool& found) const
{
    event = RemoteWindingEvent();
    found = false;
    if (!m_storage.exists(EventsPath)) return true;

    File file = m_storage.open(EventsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize == 0U)
    {
        file.close();
        return true;
    }
    if (rawSize > 0xFFFFFFFFUL)
    {
        file.close();
        return false;
    }

    // Records produced by this archive are well below 512 bytes. Read only the
    // bounded tail instead of rescanning the complete append-only file for every
    // UART retry or completion. A longer unterminated record fails closed.
    constexpr size_t TailBytes = 512U;
    const uint32_t fileSize = static_cast<uint32_t>(rawSize);
    const size_t readLength = rawSize < TailBytes ? rawSize : TailBytes;
    const uint32_t start = fileSize - static_cast<uint32_t>(readLength);
    if (!file.seek(start))
    {
        file.close();
        return false;
    }

    char buffer[TailBytes + 1U];
    const size_t readCount =
        file.read(reinterpret_cast<uint8_t*>(buffer), readLength);
    file.close();
    if (readCount != readLength || readLength == 0U ||
        buffer[readLength - 1U] != '\n')
    {
        return false;
    }

    size_t end = readLength - 1U;
    while (end > 0U && buffer[end - 1U] == '\n') --end;
    if (end == 0U)
    {
        return start == 0UL;
    }

    int previousNewline = -1;
    for (int index = static_cast<int>(end) - 1; index >= 0; --index)
    {
        if (buffer[index] == '\n')
        {
            previousNewline = index;
            break;
        }
    }
    if (previousNewline < 0 && start > 0UL) return false;

    buffer[end] = '\0';
    const char* record = buffer + (previousNewline >= 0 ? previousNewline + 1 : 0);
    const String line(record);
    if (line.length() == 0U || !FlatJsonObjectValidator::valid(line) ||
        !parseEventRecord(line, event))
    {
        return false;
    }

    found = true;
    return true;
}

bool AutonomousWindingArchive::findEventReplay(const RemoteWindingEvent& event,
                                               bool& exactMatch,
                                               bool& conflict) const
{
    exactMatch = false;
    conflict = false;

    RemoteWindingEvent latest;
    bool found = false;
    if (!loadLastEvent(latest, found)) return false;
    if (!found) return true;

    if (event.runId < latest.runId ||
        (event.runId > latest.runId && event.sessionId < latest.sessionId))
    {
        conflict = true;
        return true;
    }
    if (event.runId > latest.runId) return true;

    if (event.sessionId != latest.sessionId ||
        event.jobType != latest.jobType ||
        event.coilCount != latest.coilCount)
    {
        conflict = true;
        return true;
    }
    for (uint8_t index = 0U; index < event.coilCount; ++index)
    {
        if (event.turns[index] != latest.turns[index])
        {
            conflict = true;
            return true;
        }
    }

    if (event.type == latest.type)
    {
        if (event.completedRuns != latest.completedRuns)
            conflict = true;
        else
            exactMatch = true;
        return true;
    }

    // The only valid same-run non-replay transition is START -> COMPLETE.
    if (latest.type == RemoteEventType::RunStarted &&
        event.type == RemoteEventType::RunCompleted)
    {
        return true;
    }

    conflict = true;
    return true;
}

bool AutonomousWindingArchive::matchingStartExists(const RemoteWindingEvent& event,
                                                    bool& found) const
{
    found = false;

    RemoteWindingEvent latest;
    bool latestFound = false;
    if (!loadLastEvent(latest, latestFound)) return false;
    if (!latestFound) return true;

    if (latest.type != RemoteEventType::RunStarted ||
        latest.sessionId != event.sessionId || latest.runId != event.runId ||
        latest.jobType != event.jobType || latest.coilCount != event.coilCount)
    {
        return true;
    }

    for (uint8_t index = 0U; index < event.coilCount; ++index)
    {
        if (latest.turns[index] != event.turns[index]) return true;
    }

    found = true;
    return true;
}

bool AutonomousWindingArchive::nextAssignmentId(uint32_t& assignmentId) const
{
    assignmentId = 1UL;
    if (!m_storage.exists(AssignmentsPath)) return true;
    File file = m_storage.open(AssignmentsPath, FILE_READ);
    if (!file) return false;
    uint32_t highest = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        AutonomousWindingAssignment assignment;
        if (!FlatJsonObjectValidator::valid(line) || !parseAssignment(line, assignment) ||
            assignment.assignmentId <= highest)
        {
            file.close();
            return false;
        }
        highest = assignment.assignmentId;
    }
    file.close();
    if (highest == 0xFFFFFFFFUL) return false;
    assignmentId = highest + 1UL;
    return true;
}

bool AutonomousWindingArchive::parseEventRecord(const String& line,
                                                RemoteWindingEvent& event)
{
    event = RemoteWindingEvent();
    uint32_t schemaVersion = 0UL;
    uint32_t coilCount = 0UL;
    uint32_t completedRuns = 0UL;
    uint32_t startObserved = 0UL;
    uint32_t uptimeMs = 0UL;
    String source;
    String eventText;
    String windingType;
    String program;
    if (!findUnsigned(line, "schema_version", schemaVersion) || schemaVersion != 1UL ||
        !findString(line, "source", source) || source != "ARDUINO_LOCAL" ||
        !findUnsigned(line, "session_id", event.sessionId) || event.sessionId == 0UL ||
        !findUnsigned(line, "run_id", event.runId) || event.runId == 0UL ||
        !findString(line, "event", eventText) ||
        !findString(line, "winding_type", windingType) ||
        !findUnsigned(line, "coil_count", coilCount) || coilCount == 0UL || coilCount > 10UL ||
        !findString(line, "program", program) ||
        !findUnsigned(line, "completed_runs", completedRuns) || completedRuns > 0xFFFFUL ||
        !findUnsigned(line, "start_observed", startObserved) || startObserved > 1UL ||
        !findUnsigned(line, "received_uptime_ms", uptimeMs))
    {
        return false;
    }

    if (eventText == "RUN_STARTED") event.type = RemoteEventType::RunStarted;
    else if (eventText == "RUN_COMPLETED") event.type = RemoteEventType::RunCompleted;
    else return false;

    if (event.type == RemoteEventType::RunStarted && startObserved != 1UL)
        return false;

    if (windingType == "WORKING") event.jobType = RemoteJobType::Working;
    else if (windingType == "STARTING") event.jobType = RemoteJobType::Starting;
    else return false;

    uint8_t parsedCount = 0U;
    if (!WindingProgramParser::parse(program,
                                     event.turns,
                                     RemoteWindingEvent::MaxCoils,
                                     parsedCount) ||
        parsedCount != static_cast<uint8_t>(coilCount))
    {
        return false;
    }

    event.coilCount = parsedCount;
    event.completedRuns = static_cast<uint16_t>(completedRuns);
    event.localStandalone = true;
    if ((event.type == RemoteEventType::RunStarted && event.completedRuns != 0U) ||
        (event.type == RemoteEventType::RunCompleted && event.completedRuns == 0U))
    {
        return false;
    }
    return event.hasProgram();
}

bool AutonomousWindingArchive::parseAssignment(
    const String& line,
    AutonomousWindingAssignment& assignment)
{
    assignment = AutonomousWindingAssignment();
    uint32_t schemaVersion = 0UL;
    uint32_t uptimeMs = 0UL;
    if (!findUnsigned(line, "schema_version", schemaVersion) || schemaVersion != 1UL ||
        !findUnsigned(line, "assignment_id", assignment.assignmentId) ||
        assignment.assignmentId == 0UL ||
        !findUnsigned(line, "session_id", assignment.sessionId) || assignment.sessionId == 0UL ||
        !findUnsigned(line, "run_id", assignment.runId) || assignment.runId == 0UL ||
        !findUnsigned(line, "motor_id", assignment.motorId) || assignment.motorId == 0UL ||
        !findString(line, "role", assignment.role) || !validRole(assignment.role) ||
        !findUnsigned(line, "assigned_uptime_ms", uptimeMs))
    {
        return false;
    }
    return assignment.isValid();
}

bool AutonomousWindingArchive::programMatches(const String& candidate,
                                              const String& query,
                                              uint8_t tolerancePercent)
{
    uint16_t candidateTurns[10] = {};
    uint16_t queryTurns[10] = {};
    uint8_t candidateCount = 0U;
    uint8_t queryCount = 0U;
    if (!WindingProgramParser::parse(candidate, candidateTurns, 10U, candidateCount) ||
        !WindingProgramParser::parse(query, queryTurns, 10U, queryCount) ||
        candidateCount != queryCount)
    {
        return false;
    }

    for (uint8_t index = 0U; index < queryCount; ++index)
    {
        const uint32_t a = candidateTurns[index];
        const uint32_t b = queryTurns[index];
        const uint32_t difference = a > b ? a - b : b - a;
        if (difference * 100UL > b * static_cast<uint32_t>(tolerancePercent))
            return false;
    }
    return true;
}

String AutonomousWindingArchive::programText(const RemoteWindingEvent& event)
{
    String result;
    result.reserve(static_cast<unsigned int>(event.coilCount) * 5U);
    for (uint8_t index = 0U; index < event.coilCount; ++index)
    {
        if (index > 0U) result += '/';
        result += event.turns[index];
    }
    return result;
}

bool AutonomousWindingArchive::validRole(const String& role)
{
    return role == "WORKING" || role == "STARTING" || role == "AUXILIARY";
}

bool AutonomousWindingArchive::findUnsigned(const String& line,
                                            const char* key,
                                            uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0) return false;
    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool AutonomousWindingArchive::findString(const String& line,
                                          const char* key,
                                          String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0) return false;
    const int start = position + marker.length();
    int end = start;
    bool escaped = false;
    while (end < line.length())
    {
        const char ch = line[end];
        if (!escaped && ch == '"') break;
        if (!escaped && ch == '\\') escaped = true;
        else escaped = false;
        ++end;
    }
    if (end >= line.length()) return false;
    int cursor = end + 1;
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = line.substring(start, end);
    return true;
}

String AutonomousWindingArchive::jsonEscape(const String& value)
{
    String result;
    result.reserve(value.length() + 8U);
    for (size_t index = 0U; index < value.length(); ++index)
    {
        const char ch = value[index];
        if (ch == '"' || ch == '\\') result += '\\';
        result += ch;
    }
    return result;
}

const char* AutonomousWindingArchive::eventName(RemoteEventType type)
{
    return type == RemoteEventType::RunCompleted ? "RUN_COMPLETED" : "RUN_STARTED";
}

const char* AutonomousWindingArchive::windingTypeName(RemoteJobType type)
{
    return type == RemoteJobType::Starting ? "STARTING" : "WORKING";
}
}
