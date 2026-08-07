#include "CM_WindingJournal.h"

#include <stdlib.h>
#include <string.h>

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
    return m_ready;
}

JournalSaveResult WindingJournal::save(const RemoteWindingEvent& event,
                                       const WindingEventContext& context)
{
    if (!m_ready)
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

    if (containsRunEvent(event.sessionId, event.runId, event.type))
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
        if (!hasRunStart(event.sessionId, event.runId) ||
            event.completedRuns == 0U)
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
    if (!m_ready || sessionId == 0UL) return false;

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
        if (line.length() == 0U || line[0] != '{' ||
            line[line.length() - 1U] != '}')
        {
            file.close();
            return false;
        }

        uint32_t schemaVersion = 0UL;
        uint32_t runId = 0UL;
        uint32_t sessionId = 0UL;
        uint32_t completedRuns = 0UL;
        uint32_t uptimeMs = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL) ||
            !findUnsigned(line, "run_id", runId) || runId == 0UL ||
            !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
            !findUnsigned(line, "completed_runs", completedRuns) ||
            completedRuns > 0xFFFFUL ||
            !findUnsigned(line, "uptime_ms", uptimeMs))
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

        const bool started =
            line.indexOf(F("\"event\":\"RUN_STARTED\"")) >= 0;
        const bool completed =
            line.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0;
        if (started == completed ||
            (started && completedRuns != 0UL) ||
            (completed && completedRuns == 0UL))
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

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t schemaVersion = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            schemaVersion != 2UL)
        {
            continue;
        }

        uint32_t sessionId = 0UL;
        WindingEventContext context;
        if (!findUnsigned(line, "session_id", sessionId) ||
            sessionId == 0UL || !parseContext(line, context) ||
            !sessionContextMatches(sessionId, context))
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
        uint32_t lineSessionId = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            schemaVersion != 2UL ||
            !findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId != sessionId)
        {
            continue;
        }

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
                                      RemoteEventType type) const
{
    if (!m_fileSystem.exists(JournalPath)) return false;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.indexOf(String("\"event\":\"") + eventTypeName(type) + '"') < 0)
            continue;

        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        if (findUnsigned(line, "session_id", lineSessionId) &&
            lineSessionId == sessionId &&
            findUnsigned(line, "run_id", lineRunId) &&
            lineRunId == runId)
        {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

bool WindingJournal::hasRunStart(uint32_t sessionId, uint32_t runId) const
{
    return containsRunEvent(sessionId, runId, RemoteEventType::RunStarted);
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
        if (line.indexOf(F("\"event\":\"RUN_COMPLETED\"")) < 0)
            continue;

        uint32_t lineSessionId = 0UL;
        uint32_t lineCompletedRuns = 0UL;
        if (!findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId != sessionId ||
            !findUnsigned(line, "completed_runs", lineCompletedRuns) ||
            lineCompletedRuns > 0xFFFFUL)
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
        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        if (!findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId != sessionId ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL)
        {
            continue;
        }

        if (line.indexOf(F("\"event\":\"RUN_STARTED\"")) >= 0)
        {
            if (found && runId != lineRunId)
            {
                file.close();
                return false;
            }
            runId = lineRunId;
            found = true;
        }
        else if (line.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0 &&
                 found && runId == lineRunId)
        {
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
        if (line.indexOf(F("\"event\":\"RUN_STARTED\"")) < 0)
            continue;

        uint32_t lineSessionId = 0UL;
        uint32_t lineRunId = 0UL;
        if (!findUnsigned(line, "session_id", lineSessionId) ||
            lineSessionId != sessionId ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId == 0UL)
        {
            continue;
        }

        if (lineRunId > highestRunId) highestRunId = lineRunId;
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

bool WindingJournal::findUnsigned(const String& line,
                                  const char* key,
                                  uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0) return false;

    int start = position + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    int end = start;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == start) return false;

    const String number = line.substring(start, end);
    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(number.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0') return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool WindingJournal::fieldIsNull(const String& line, const char* key)
{
    const String marker = String("\"") + key + F("\":null");
    return line.indexOf(marker) >= 0;
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
