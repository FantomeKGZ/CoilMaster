#include "CM_WindingJournalHealth.h"

#include <stdlib.h>

namespace CM
{
WindingJournalHealth::WindingJournalHealth(fs::FS& fileSystem)
    : m_fileSystem(fileSystem)
{
}

bool WindingJournalHealth::inspect(WindingJournalHealthReport& report) const
{
    report = WindingJournalHealthReport();

    if (!m_fileSystem.exists(JournalPath))
    {
        report.readable = true;
        return true;
    }

    File file = m_fileSystem.open(JournalPath, FILE_READ);
    if (!file)
    {
        return false;
    }

    uint32_t activeRunId = 0UL;
    uint32_t activeSessionId = 0UL;
    uint32_t counterSessionId = 0UL;
    uint16_t lastCompletedRuns = 0U;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        ++report.recordCount;

        uint32_t schemaVersion = 0UL;
        uint32_t runId = 0UL;
        uint32_t sessionId = 0UL;
        uint32_t completedRuns = 0UL;
        const bool started =
            line.indexOf(F("\"event\":\"RUN_STARTED\"")) >= 0;
        const bool completed =
            line.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0;

        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            schemaVersion != 1UL || started == completed ||
            !findUnsigned(line, "run_id", runId) || runId == 0UL ||
            !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
            !findUnsigned(line, "completed_runs", completedRuns) ||
            completedRuns > 0xFFFFUL)
        {
            ++report.malformedRecordCount;
            report.sequenceConsistent = false;
            continue;
        }

        if (started)
        {
            ++report.startedCount;
            if (completedRuns != 0UL || activeRunId != 0UL)
            {
                report.sequenceConsistent = false;
                continue;
            }

            if (counterSessionId != sessionId)
            {
                counterSessionId = sessionId;
                lastCompletedRuns = 0U;
            }

            activeRunId = runId;
            activeSessionId = sessionId;
            continue;
        }

        ++report.completedCount;
        if (activeRunId == 0UL || runId != activeRunId ||
            sessionId != activeSessionId || sessionId != counterSessionId ||
            completedRuns == 0UL ||
            completedRuns != static_cast<uint32_t>(lastCompletedRuns) + 1UL)
        {
            report.sequenceConsistent = false;
            continue;
        }

        lastCompletedRuns = static_cast<uint16_t>(completedRuns);
        activeRunId = 0UL;
        activeSessionId = 0UL;
    }

    file.close();
    report.activeRunCount = activeRunId == 0UL ? 0UL : 1UL;
    report.readable = true;
    return true;
}

bool WindingJournalHealth::findUnsigned(const String& line,
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

    const String text = line.substring(start, end);
    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(text.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0') return false;

    value = static_cast<uint32_t>(parsed);
    return true;
}
}
