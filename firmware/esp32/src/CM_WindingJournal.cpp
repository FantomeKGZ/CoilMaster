#include "CM_WindingJournal.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
WindingJournal::WindingJournal(fs::FS& fileSystem)
    : m_fileSystem(fileSystem), m_ready(false)
{
}

bool WindingJournal::begin()
{
    m_ready = ensureDirectories();
    if (m_ready) m_ready = validateJournalStructure();
    if (m_ready) m_ready = validateJournalSessionContexts();
    return m_ready;
}

bool WindingJournal::isReady() const
{
    if (!m_ready) return false;
    File directory = m_fileSystem.open(DirectoryPath, FILE_READ);
    if (!directory) return false;
    const bool ready = directory.isDirectory();
    directory.close();
    return ready;
}

JournalSaveResult WindingJournal::save(const RemoteWindingEvent& event,
                                       const WindingEventContext& context)
{
    if (!isReady())
        return JournalSaveResult::StorageUnavailable;

    if (!context.isValid() || event.runId == 0UL ||
        event.sessionId == 0UL || event.type == RemoteEventType::None)
    {
        return JournalSaveResult::InvalidTransition;
    }

    if (!sessionContextMatches(event.sessionId, context))
        return JournalSaveResult::InvalidTransition;

    if (event.type == RemoteEventType::RunStarted && event.completedRuns != 0U)
        return JournalSaveResult::InvalidTransition;

    // A replay is idempotent only when the persisted event semantics match
    // exactly. Same session/run/type with a different completed_runs value must
    // continue through transition validation and fail closed rather than being
    // treated as a harmless duplicate.
    bool duplicateFound = false;
    if (!containsRunEvent(event.sessionId,
                          event.runId,
                          event.type,
                          event.completedRuns,
                          duplicateFound))
    {
        return JournalSaveResult::InvalidTransition;
    }
    if (duplicateFound)
        return JournalSaveResult::Duplicate;

    if (event.type == RemoteEventType::RunStarted)
    {
        uint32_t activeRunId = 0UL;
        bool activeRunFound = false;
        if (!loadActiveRun(event.sessionId, activeRunId, activeRunFound) ||
            activeRunFound)
        {
            return JournalSaveResult::InvalidTransition;
        }

        uint32_t highestRunId = 0UL;
        if (!loadSessionHighestRunId(event.sessionId, highestRunId) ||
            event.runId <= highestRunId)
        {
            return JournalSaveResult::InvalidTransition;
        }
    }

    if (event.type == RemoteEventType::RunCompleted)
    {
        bool runStartFound = false;
        if (!hasRunStart(event.sessionId, event.runId, runStartFound) ||
            !runStartFound || event.completedRuns == 0U)
        {
            return JournalSaveResult::InvalidTransition;
        }

        uint32_t activeRunId = 0UL;
        bool activeRunFound = false;
        if (!loadActiveRun(event.sessionId, activeRunId, activeRunFound) ||
            !activeRunFound || activeRunId != event.runId)
        {
            return JournalSaveResult::InvalidTransition;
        }

        uint16_t previousCompletedRuns = 0U;
        if (!loadSessionCompletedRuns(event.sessionId, previousCompletedRuns) ||
            previousCompletedRuns == 0xFFFFU ||
            event.completedRuns != static_cast<uint16_t>(previousCompletedRuns + 1U))
        {
            return JournalSaveResult::InvalidTransition;
        }
    }

    return appendRecord(event, context)
               ? JournalSaveResult::Saved
               : JournalSaveResult::WriteFailed;
}

bool WindingJournal::loadSessionState(uint32_t sessionId,
                                      WindingSessionState& state) const
{
    state = WindingSessionState();
    state.sessionId = sessionId;
    if (!isReady() || sessionId == 0UL) return false;

    uint32_t activeRunId = 0UL;
    bool activeRunFound = false;
    uint32_t highestRunId = 0UL;
    uint16_t completedRuns = 0U;
    if (!loadActiveRun(sessionId, activeRunId, activeRunFound) ||
        !loadSessionHighestRunId(sessionId, highestRunId) ||
        !loadSessionCompletedRuns(sessionId, completedRuns))
    {
        return false;
    }

    if ((activeRunFound && (activeRunId == 0UL || activeRunId > highestRunId)) ||
        (!activeRunFound && activeRunId != 0UL))
    {
        return false;
    }

    state.activeRunId = activeRunId;
    state.highestRunId = highestRunId;
    state.completedRuns = completedRuns;
    state.activeRunFound = activeRunFound;
    state.journalConsistent = true;
    return true;
}

bool WindingJournal::ensureDirectories()
{
    if (!m_fileSystem.exists("/data") && !m_fileSystem.mkdir("/data"))
        return false;
    if (!m_fileSystem.exists(DirectoryPath) &&
        !m_fileSystem.mkdir(DirectoryPath))
        return false;
    return true;
}

bool WindingJournal::validateJournalStructure() const
{
    if (!m_fileSystem.exists(JournalPath)) return true;

    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U || !FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t schemaVersion = 0UL;
        uint32_t runId = 0UL;
        uint32_t sessionId = 0UL;
        uint32_t completedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType eventType = RemoteEventType::None;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "run_id", runId) || runId == 0UL ||
            !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
            !findUnsigned(line, "completed_runs", completedRuns) ||
            completedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs) ||
            !parseEvent(line, eventType))
        {
            file.close();
            return false;
        }

        if (schemaVersion == 2UL)
        {
            WindingEventContext context;
            if (!parseContext(line, context))
            {
                file.close();
                return false;
            }
        }

        if ((eventType == RemoteEventType::RunStarted && completedRuns != 0UL) ||
            (eventType == RemoteEventType::RunCompleted && completedRuns == 0UL))
        {
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool WindingJournal::validateJournalSessionContexts() const
{
    if (!m_fileSystem.exists(JournalPath)) return true;

    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    uint32_t currentSessionId = 0UL;
    WindingEventContext currentContext;
    bool haveCurrentContext = false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion))
        {
            file.close();
            return false;
        }
        if (schemaVersion == 1UL) continue;
        if (schemaVersion != 2UL)
        {
            file.close();
            return false;
        }

        uint32_t sessionId = 0UL;
        WindingEventContext context;
        if (!findUnsigned(line, "session_id", sessionId) ||
            sessionId == 0UL || !parseContext(line, context))
        {
            file.close();
            return false;
        }

        if (!haveCurrentContext)
        {
            currentSessionId = sessionId;
            currentContext = context;
            haveCurrentContext = true;
            continue;
        }

        if (sessionId < currentSessionId)
        {
            file.close();
            return false;
        }

        if (sessionId > currentSessionId)
        {
            currentSessionId = sessionId;
            currentContext = context;
            continue;
        }

        if (context.jobId != currentContext.jobId ||
            context.linked != currentContext.linked ||
            context.repairId != currentContext.repairId ||
            context.motorId != currentContext.motorId)
        {
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool WindingJournal::sessionContextMatches(
    uint32_t sessionId,
    const WindingEventContext& context) const
{
    if (sessionId == 0UL || !context.isValid()) return false;
    if (!m_fileSystem.exists(JournalPath)) return true;

    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion))
        {
            file.close();
            return false;
        }
        if (schemaVersion == 1UL) continue;
        if (schemaVersion != 2UL)
        {
            file.close();
            return false;
        }

        uint32_t lineSessionId = 0UL;
        if (!findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId == 0UL)
        {
            file.close();
            return false;
        }

        if (lineSessionId > sessionId)
        {
            file.close();
            return false;
        }
        if (lineSessionId != sessionId)
            continue;

        WindingEventContext existing;
        if (!parseContext(line, existing) ||
            existing.jobId != context.jobId ||
            existing.linked != context.linked ||
            existing.repairId != context.repairId ||
            existing.motorId != context.motorId)
        {
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool WindingJournal::containsRunEvent(uint32_t sessionId,
                                      uint32_t runId,
                                      RemoteEventType type,
                                      uint16_t completedRuns,
                                      bool& found) const
{
    found = false;
    if (!m_fileSystem.exists(JournalPath)) return true;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType lineType = RemoteEventType::None;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId == 0UL ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL ||
            !findUnsigned(line, "completed_runs", lineCompletedRuns) ||
            lineCompletedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs) ||
            !parseEvent(line, lineType))
        {
            file.close();
            return false;
        }

        if (schemaVersion == 2UL)
        {
            WindingEventContext context;
            if (!parseContext(line, context))
            {
                file.close();
                return false;
            }
        }

        if ((lineType == RemoteEventType::RunStarted && lineCompletedRuns != 0UL) ||
            (lineType == RemoteEventType::RunCompleted && lineCompletedRuns == 0UL))
        {
            file.close();
            return false;
        }

        if (lineSessionId == sessionId &&
            lineRunId == runId &&
            lineType == type &&
            lineCompletedRuns == completedRuns)
        {
            found = true;
            file.close();
            return true;
        }
    }

    file.close();
    return true;
}

bool WindingJournal::hasRunStart(uint32_t sessionId,
                                 uint32_t runId,
                                 bool& found) const
{
    return containsRunEvent(sessionId,
                            runId,
                            RemoteEventType::RunStarted,
                            0U,
                            found);
}

bool WindingJournal::loadSessionCompletedRuns(uint32_t sessionId,
                                              uint16_t& completedRuns) const
{
    completedRuns = 0U;
    if (!m_fileSystem.exists(JournalPath)) return true;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType lineType = RemoteEventType::None;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId == 0UL ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL ||
            !findUnsigned(line, "completed_runs", lineCompletedRuns) ||
            lineCompletedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs) ||
            !parseEvent(line, lineType))
        {
            file.close();
            return false;
        }

        if (schemaVersion == 2UL)
        {
            WindingEventContext context;
            if (!parseContext(line, context))
            {
                file.close();
                return false;
            }
        }

        if ((lineType == RemoteEventType::RunStarted && lineCompletedRuns != 0UL) ||
            (lineType == RemoteEventType::RunCompleted && lineCompletedRuns == 0UL))
        {
            file.close();
            return false;
        }

        if (lineSessionId != sessionId ||
            lineType != RemoteEventType::RunCompleted)
        {
            continue;
        }

        const uint16_t candidate = static_cast<uint16_t>(lineCompletedRuns);
        if (candidate > completedRuns) completedRuns = candidate;
    }

    file.close();
    return true;
}

bool WindingJournal::loadActiveRun(uint32_t sessionId,
                                   uint32_t& runId,
                                   bool& found) const
{
    runId = 0UL;
    found = false;
    if (!m_fileSystem.exists(JournalPath)) return true;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType lineType = RemoteEventType::None;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId == 0UL ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL ||
            !findUnsigned(line, "completed_runs", lineCompletedRuns) ||
            lineCompletedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs) ||
            !parseEvent(line, lineType))
        {
            file.close();
            return false;
        }

        if (schemaVersion == 2UL)
        {
            WindingEventContext context;
            if (!parseContext(line, context))
            {
                file.close();
                return false;
            }
        }

        if ((lineType == RemoteEventType::RunStarted && lineCompletedRuns != 0UL) ||
            (lineType == RemoteEventType::RunCompleted && lineCompletedRuns == 0UL))
        {
            file.close();
            return false;
        }

        if (lineSessionId != sessionId) continue;

        if (lineType == RemoteEventType::RunStarted)
        {
            if (found)
            {
                file.close();
                return false;
            }
            runId = lineRunId;
            found = true;
        }
        else
        {
            if (!found || runId != lineRunId)
            {
                file.close();
                return false;
            }
            runId = 0UL;
            found = false;
        }
    }

    file.close();
    return true;
}

bool WindingJournal::loadSessionHighestRunId(uint32_t sessionId,
                                             uint32_t& highestRunId) const
{
    highestRunId = 0UL;
    if (!m_fileSystem.exists(JournalPath)) return true;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType lineType = RemoteEventType::None;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId == 0UL ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL ||
            !findUnsigned(line, "completed_runs", lineCompletedRuns) ||
            lineCompletedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs) ||
            !parseEvent(line, lineType))
        {
            file.close();
            return false;
        }

        if (schemaVersion == 2UL)
        {
            WindingEventContext context;
            if (!parseContext(line, context))
            {
                file.close();
                return false;
            }
        }

        if ((lineType == RemoteEventType::RunStarted && lineCompletedRuns != 0UL) ||
            (lineType == RemoteEventType::RunCompleted && lineCompletedRuns == 0UL))
        {
            file.close();
            return false;
        }

        if (lineSessionId != sessionId ||
            lineType != RemoteEventType::RunStarted)
        {
            continue;
        }

        if (lineRunId <= highestRunId)
        {
            file.close();
            return false;
        }
        highestRunId = lineRunId;
    }

    file.close();
    return true;
}

bool WindingJournal::appendRecord(const RemoteWindingEvent& event,
                                  const WindingEventContext& context)
{
    File file = m_fileSystem.open(JournalPath, FILE_APPEND);
    if (!file) return false;

    String record;
    record.reserve(240U);
    record = F("{\"schema_version\":2,\"job_id\":");
    record += context.jobId;
    record += F(",\"session_id\":");
    record += event.sessionId;
    record += F(",\"run_id\":");
    record += event.runId;
    record += F(",\"event\":\"");
    record += eventTypeName(event.type);
    record += F("\",\"repair_id\":");
    if (context.linked) record += context.repairId;
    else record += F("null");
    record += F(",\"motor_id\":");
    if (context.linked) record += context.motorId;
    else record += F("null");
    record += F(",\"completed_runs\":");
    record += event.completedRuns;
    record += F(",\"uptime_ms\":");
    record += millis();
    record += F("}\n");

    const size_t written = file.print(record);
    file.flush();
    file.close();
    return written == record.length();
}

bool WindingJournal::parseContext(const String& line,
                                  WindingEventContext& context)
{
    context = WindingEventContext();
    if (!findUnsigned(line, "job_id", context.jobId) ||
        context.jobId == 0UL)
    {
        return false;
    }

    const bool repairNull = fieldIsNull(line, "repair_id");
    const bool motorNull = fieldIsNull(line, "motor_id");
    const bool repairValue = findUnsigned(line, "repair_id", context.repairId) &&
                             context.repairId != 0UL;
    const bool motorValue = findUnsigned(line, "motor_id", context.motorId) &&
                            context.motorId != 0UL;

    if (repairNull && motorNull)
    {
        context.linked = false;
        context.repairId = 0UL;
        context.motorId = 0UL;
    }
    else if (repairValue && motorValue)
    {
        context.linked = true;
    }
    else
    {
        return false;
    }

    return context.isValid();
}

bool WindingJournal::parseEvent(const String& line, RemoteEventType& type)
{
    type = RemoteEventType::None;
    const String marker = F("\"event\":\"");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
        return false;

    const int start = position + marker.length();
    const int end = line.indexOf('"', start);
    if (end <= start) return false;

    int cursor = end + 1;
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}'))
    {
        return false;
    }

    const String value = line.substring(start, end);
    if (value == "RUN_STARTED")
        type = RemoteEventType::RunStarted;
    else if (value == "RUN_COMPLETED")
        type = RemoteEventType::RunCompleted;
    else
        return false;
    return true;
}

bool WindingJournal::findUnsigned(const String& line,
                                  const char* key,
                                  uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
        return false;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() &&
        isDigit(line[cursor + 1])) return false;

    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;

    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}'))
    {
        return false;
    }

    value = parsed;
    return true;
}

bool WindingJournal::fieldIsNull(const String& line, const char* key)
{
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
        return false;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor + 4 > line.length() || line.substring(cursor, cursor + 4) != "null")
        return false;
    cursor += 4;
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    return cursor < line.length() &&
           (line[cursor] == ',' || line[cursor] == '}');
}

const char* WindingJournal::eventTypeName(RemoteEventType type)
{
    switch (type)
    {
        case RemoteEventType::RunStarted: return "RUN_STARTED";
        case RemoteEventType::RunCompleted: return "RUN_COMPLETED";
        case RemoteEventType::None:
        default: return "NONE";
    }
}
}
