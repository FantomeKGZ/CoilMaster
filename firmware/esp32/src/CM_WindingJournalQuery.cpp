#include "CM_WindingJournalQuery.h"

#include <stdlib.h>

namespace CM
{
WindingJournalQuery::WindingJournalQuery(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool WindingJournalQuery::begin()
{
    m_ready = true;
    return true;
}

bool WindingJournalQuery::isReady() const
{
    return m_ready;
}

WindingJournalQueryResult WindingJournalQuery::appendHistoryJson(
    uint32_t sessionId,
    uint32_t repairId,
    uint16_t limit,
    String& json,
    uint16_t& count) const
{
    count = 0U;
    if (!m_ready)
        return WindingJournalQueryResult::StorageUnavailable;
    if ((sessionId == 0UL) == (repairId == 0UL))
        return WindingJournalQueryResult::InvalidFilter;

    if (limit == 0U) limit = 1U;
    if (limit > 100U) limit = 100U;
    if (!m_storage.exists(JournalPath)) return WindingJournalQueryResult::Ok;

    File file = m_storage.open(JournalPath, FILE_READ);
    if (!file) return WindingJournalQueryResult::ReadFailed;

    bool first = true;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        uint32_t lineSessionId = 0UL;
        uint32_t lineRepairId = 0UL;
        bool linked = false;
        if (!isValidSchema2Record(line, lineSessionId, linked, lineRepairId))
            continue;

        const bool matches = sessionId != 0UL
            ? lineSessionId == sessionId
            : linked && lineRepairId == repairId;
        if (!matches) continue;

        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }

    file.close();
    return WindingJournalQueryResult::Ok;
}

bool WindingJournalQuery::findUnsigned(const String& line,
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

bool WindingJournalQuery::fieldIsNull(const String& line, const char* key)
{
    return line.indexOf(String("\"") + key + F("\":null")) >= 0;
}

bool WindingJournalQuery::isValidSchema2Record(const String& line,
                                               uint32_t& sessionId,
                                               bool& linked,
                                               uint32_t& repairId)
{
    sessionId = 0UL;
    repairId = 0UL;
    linked = false;

    uint32_t schemaVersion = 0UL;
    uint32_t jobId = 0UL;
    uint32_t runId = 0UL;
    uint32_t completedRuns = 0UL;
    uint32_t uptimeMs = 0UL;
    uint32_t motorId = 0UL;
    if (line.length() < 2U || line[0] != '{' ||
        line[line.length() - 1U] != '}' ||
        !findUnsigned(line, "schema_version", schemaVersion) ||
        schemaVersion != 2UL ||
        !findUnsigned(line, "job_id", jobId) || jobId == 0UL ||
        !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
        !findUnsigned(line, "run_id", runId) || runId == 0UL ||
        !findUnsigned(line, "completed_runs", completedRuns) ||
        completedRuns > 0xFFFFUL ||
        !findUnsigned(line, "uptime_ms", uptimeMs))
    {
        return false;
    }

    const bool started = line.indexOf(F("\"event\":\"RUN_STARTED\"")) >= 0;
    const bool completed = line.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0;
    if (started == completed ||
        (started && completedRuns != 0UL) ||
        (completed && completedRuns == 0UL))
    {
        return false;
    }

    const bool repairNull = fieldIsNull(line, "repair_id");
    const bool motorNull = fieldIsNull(line, "motor_id");
    const bool repairValue = findUnsigned(line, "repair_id", repairId) && repairId != 0UL;
    const bool motorValue = findUnsigned(line, "motor_id", motorId) && motorId != 0UL;
    if (repairNull && motorNull)
    {
        repairId = 0UL;
        linked = false;
        return true;
    }
    if (repairValue && motorValue)
    {
        linked = true;
        return true;
    }
    return false;
}
}
