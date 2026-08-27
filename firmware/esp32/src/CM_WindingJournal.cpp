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

    if ((event.type == RemoteEventType::RunStarted && event.completedRuns != 0U) ||
        (event.type == RemoteEventType::RunCompleted && event.completedRuns == 0U))
    {
        return JournalSaveResult::InvalidTransition;
    }

    WindingSessionState state;
    bool duplicateFound = false;
    bool runStartFound = false;
    if (!analyzeSession(event.sessionId,
                        event.runId,
                        event.type,
                        event.completedRuns,
                        &context,
                        state,
                        duplicateFound,
                        runStartFound))
    {
        return JournalSaveResult::InvalidTransition;
    }

    // Exact replay remains idempotent. analyzeSession keeps scanning schema-2
    // context after finding it so context corruption cannot be hidden by replay.
    if (duplicateFound)
        return JournalSaveResult::Duplicate;

    if (event.type == RemoteEventType::RunStarted)
    {
        if (state.activeRunFound || event.runId <= state.highestRunId)
            return JournalSaveResult::InvalidTransition;
    }
    else if (event.type == RemoteEventType::RunCompleted)
    {
        if (!runStartFound || !state.activeRunFound ||
            state.activeRunId != event.runId ||
            state.completedRuns == 0xFFFFU ||
            event.completedRuns != static_cast<uint16_t>(state.completedRuns + 1U))
        {
            return JournalSaveResult::InvalidTransition;
        }
    }
    else
    {
        return JournalSaveResult::InvalidTransition;
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

    bool ignoredExactEvent = false;
    bool ignoredRunStart = false;
    return analyzeSession(sessionId,
                          0UL,
                          RemoteEventType::None,
                          0U,
                          nullptr,
                          state,
                          ignoredExactEvent,
                          ignoredRunStart);
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

bool WindingJournal::analyzeSession(
    uint32_t sessionId,
    uint32_t targetRunId,
    RemoteEventType targetType,
    uint16_t targetCompletedRuns,
    const WindingEventContext* expectedContext,
    WindingSessionState& state,
    bool& exactEventFound,
    bool& targetRunStartFound) const
{
    state = WindingSessionState();
    state.sessionId = sessionId;
    exactEventFound = false;
    targetRunStartFound = false;

    if (sessionId == 0UL ||
        (expectedContext != nullptr && !expectedContext->isValid()))
    {
        return false;
    }

    if (!m_fileSystem.exists(JournalPath))
    {
        state.journalConsistent = true;
        return true;
    }

    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    bool stateValid = true;
    bool duplicateSeen = false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion))
        {
            file.close();
            return false;
        }

        // The old save path performed the complete immutable-context scan before
        // duplicate detection. Once an exact replay is known, preserve that
        // behavior by validating only the remaining schema/context fields; the
        // old duplicate scan would have stopped at the replay and would not have
        // parsed unrelated later event fields.
        if (duplicateSeen)
        {
            if (schemaVersion == 1UL) continue;
            if (schemaVersion != 2UL)
            {
                file.close();
                return false;
            }

            uint32_t lineSessionId = 0UL;
            WindingEventContext lineContext;
            if (!findUnsigned(line, "session_id", lineSessionId) ||
                lineSessionId == 0UL || !parseContext(line, lineContext))
            {
                file.close();
                return false;
            }

            if (expectedContext != nullptr)
            {
                if (lineSessionId > sessionId)
                {
                    file.close();
                    return false;
                }
                if (lineSessionId == sessionId &&
                    (lineContext.jobId != expectedContext->jobId ||
                     lineContext.linked != expectedContext->linked ||
                     lineContext.repairId != expectedContext->repairId ||
                     lineContext.motorId != expectedContext->motorId))
                {
                    file.close();
                    return false;
                }
            }
            continue;
        }

        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        RemoteEventType lineType = RemoteEventType::None;
        if ((schemaVersion != 1UL && schemaVersion != 2UL) ||
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

        WindingEventContext lineContext;
        if (schemaVersion == 2UL && !parseContext(line, lineContext))
        {
            file.close();
            return false;
        }

        if ((lineType == RemoteEventType::RunStarted && lineCompletedRuns != 0UL) ||
            (lineType == RemoteEventType::RunCompleted && lineCompletedRuns == 0UL))
        {
            file.close();
            return false;
        }

        if (expectedContext != nullptr && schemaVersion == 2UL)
        {
            if (lineSessionId > sessionId)
            {
                file.close();
                return false;
            }
            if (lineSessionId == sessionId &&
                (lineContext.jobId != expectedContext->jobId ||
                 lineContext.linked != expectedContext->linked ||
                 lineContext.repairId != expectedContext->repairId ||
                 lineContext.motorId != expectedContext->motorId))
            {
                file.close();
                return false;
            }
        }

        const bool exactTarget = targetType != RemoteEventType::None &&
            targetRunId != 0UL &&
            lineSessionId == sessionId &&
            lineRunId == targetRunId &&
            lineType == targetType &&
            lineCompletedRuns == targetCompletedRuns;
        if (exactTarget)
        {
            exactEventFound = true;
            duplicateSeen = true;
            continue;
        }

        if (lineSessionId != sessionId) continue;

        if (lineType == RemoteEventType::RunStarted &&
            targetRunId != 0UL && lineRunId == targetRunId)
        {
            targetRunStartFound = true;
        }

        // Transition-state errors are remembered instead of immediately
        // returning so a later exact replay keeps the previous Duplicate
        // semantics. If no replay is found, the operation still fails closed.
        if (!stateValid) continue;

        if (lineType == RemoteEventType::RunStarted)
        {
            if (state.activeRunFound || lineRunId <= state.highestRunId)
            {
                stateValid = false;
                continue;
            }
            state.activeRunId = lineRunId;
            state.activeRunFound = true;
            state.highestRunId = lineRunId;
        }
        else
        {
            if (!state.activeRunFound || state.activeRunId != lineRunId)
            {
                stateValid = false;
                continue;
            }
            state.activeRunId = 0UL;
            state.activeRunFound = false;
            const uint16_t candidate = static_cast<uint16_t>(lineCompletedRuns);
            if (candidate > state.completedRuns)
                state.completedRuns = candidate;
        }
    }

    file.close();

    if (exactEventFound)
        return true;
    if (!stateValid ||
        (state.activeRunFound &&
         (state.activeRunId == 0UL || state.activeRunId > state.highestRunId)) ||
        (!state.activeRunFound && state.activeRunId != 0UL))
    {
        return false;
    }

    state.journalConsistent = true;
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
