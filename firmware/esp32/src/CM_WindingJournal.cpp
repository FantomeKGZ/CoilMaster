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

    if (containsRunEvent(event.runId, event.type))
    {
        return JournalSaveResult::Duplicate;
    }

    if (event.type == RemoteEventType::RunCompleted)
    {
        uint32_t startedSessionId = 0UL;
        if (!loadRunStartSession(event.runId, startedSessionId) ||
            startedSessionId != event.sessionId || event.completedRuns == 0U)
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

bool WindingJournal::containsRunEvent(uint32_t runId,
                                      RemoteEventType type) const
{
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file)
    {
        return false;
    }

    char needle[72];
    snprintf(needle,
             sizeof(needle),
             "\"run_id\":%lu,\"event\":\"%s\"",
             static_cast<unsigned long>(runId),
             eventTypeName(type));

    String line;
    while (file.available())
    {
        line = file.readStringUntil('\n');
        if (line.indexOf(needle) >= 0)
        {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

bool WindingJournal::loadRunStartSession(uint32_t runId,
                                         uint32_t& sessionId) const
{
    sessionId = 0UL;
    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t lineRunId = 0UL;
        String eventMarker = F("\"event\":\"RUN_STARTED\"");
        if (line.indexOf(eventMarker) < 0 ||
            !findUnsigned(line, "run_id", lineRunId) || lineRunId != runId ||
            !findUnsigned(line, "session_id", sessionId))
        {
            continue;
        }
        file.close();
        return sessionId > 0UL;
    }

    file.close();
    return false;
}

bool WindingJournal::appendRecord(const RemoteWindingEvent& event)
{
    File file = m_fileSystem.open(JournalPath, FILE_APPEND);
    if (!file)
    {
        return false;
    }

    const size_t written = file.printf(
        "{\"schema_version\":1,\"run_id\":%lu,\"event\":\"%s\","
        "\"session_id\":%lu,\"completed_runs\":%u,\"uptime_ms\":%lu}\n",
        static_cast<unsigned long>(event.runId),
        eventTypeName(event.type),
        static_cast<unsigned long>(event.sessionId),
        static_cast<unsigned int>(event.completedRuns),
        static_cast<unsigned long>(millis()));

    file.flush();
    file.close();
    return written > 0U;
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
