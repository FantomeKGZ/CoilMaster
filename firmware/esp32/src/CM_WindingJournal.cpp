#include "CM_WindingJournal.h"

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
    return m_ready;
}

bool WindingJournal::isReady() const
{
    return m_ready;
}

JournalSaveResult WindingJournal::save(const RemoteWindingEvent& event)
{
    if (!m_ready)
    {
        return JournalSaveResult::StorageUnavailable;
    }

    if (event.runId == 0UL || event.sessionId == 0UL ||
        event.type == RemoteEventType::None)
    {
        return JournalSaveResult::InvalidTransition;
    }

    if (event.type == RemoteEventType::RunStarted &&
        event.completedRuns != 0U)
    {
        return JournalSaveResult::InvalidTransition;
    }

    if (containsRunEvent(event.sessionId, event.runId, event.type))
    {
        return JournalSaveResult::Duplicate;
    }

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

    return appendRecord(event)
               ? JournalSaveResult::Saved
               : JournalSaveResult::WriteFailed;
}

bool WindingJournal::ensureDirectories()
{
    if (!m_fileSystem.exists("/data") && !m_fileSystem.mkdir("/data"))
    {
        return false;
    }

    if (!m_fileSystem.exists(DirectoryPath) &&
        !m_fileSystem.mkdir(DirectoryPath))
    {
        return false;
    }

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

bool WindingJournal::appendRecord(const RemoteWindingEvent& event)
{
    File file = m_fileSystem.open(JournalPath, FILE_APPEND);
    if (!file)
    {
        return false;
    }

    String record;
    record.reserve(160U);
    record = F("{\"schema_version\":1,\"run_id\":");
    record += event.runId;
    record += F(",\"event\":\"");
    record += eventTypeName(event.type);
    record += F("\",\"session_id\":");
    record += event.sessionId;
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

bool WindingJournal::findUnsigned(const String& line,
                                  const char* key,
                                  uint32_t& value)
{
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0) return false;

    int start = position + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    int end = start;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == start) return false;

    value = static_cast<uint32_t>(
        strtoul(line.substring(start, end).c_str(), nullptr, 10));
    return true;
}

const char* WindingJournal::eventTypeName(RemoteEventType type)
{
    switch (type)
    {
        case RemoteEventType::RunStarted:
            return "RUN_STARTED";

        case RemoteEventType::RunCompleted:
            return "RUN_COMPLETED";

        case RemoteEventType::None:
        default:
            return "NONE";
    }
}
}
