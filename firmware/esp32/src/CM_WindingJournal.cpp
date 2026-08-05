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

    if (containsRunEvent(event.runId, event.type))
    {
        return JournalSaveResult::Duplicate;
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
