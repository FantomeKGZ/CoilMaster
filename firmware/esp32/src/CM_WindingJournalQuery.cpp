#include "CM_WindingJournalQuery.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
WindingJournalQuery::WindingJournalQuery(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool WindingJournalQuery::begin()
{
    m_ready = false;

    File root = m_storage.open("/", FILE_READ);
    if (!root)
    {
        return false;
    }

    root.close();
    m_ready = true;
    return true;
}

bool WindingJournalQuery::isReady() const
{
    if (!m_ready) return false;
    File root = m_storage.open("/", FILE_READ);
    if (!root) return false;
    root.close();
    return true;
}

WindingJournalQueryResult WindingJournalQuery::appendHistoryJson(
    uint32_t sessionId,
    uint32_t repairId,
    uint16_t limit,
    String& json,
    uint16_t& count) const
{
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    return appendHistoryJson(sessionId,
                             repairId,
                             0UL,
                             limit,
                             json,
                             count,
                             nextCursor,
                             hasMore);
}

WindingJournalQueryResult WindingJournalQuery::appendHistoryJson(
    uint32_t sessionId,
    uint32_t repairId,
    uint32_t cursor,
    uint16_t limit,
    String& json,
    uint16_t& count,
    uint32_t& nextCursor,
    bool& hasMore) const
{
    count = 0U;
    nextCursor = cursor;
    hasMore = false;
    if (!isReady())
        return WindingJournalQueryResult::StorageUnavailable;
    if ((sessionId == 0UL) == (repairId == 0UL))
        return WindingJournalQueryResult::InvalidFilter;

    if (limit == 0U) limit = 1U;
    if (limit > 100U) limit = 100U;
    if (!m_storage.exists(JournalPath)) return WindingJournalQueryResult::Ok;

    File file = m_storage.open(JournalPath, FILE_READ);
    if (!file) return WindingJournalQueryResult::ReadFailed;

    bool first = true;
    uint32_t matchedIndex = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U)
        {
            file.close();
            return WindingJournalQueryResult::ReadFailed;
        }

        uint32_t schemaVersion = 0UL;
        if (!findUnsigned(line, "schema_version", schemaVersion) ||
            (schemaVersion != 1UL && schemaVersion != 2UL))
        {
            file.close();
            return WindingJournalQueryResult::ReadFailed;
        }

        if (schemaVersion == 1UL)
        {
            if (!isValidLegacySchema1Record(line))
            {
                file.close();
                return WindingJournalQueryResult::ReadFailed;
            }
            continue;
        }

        uint32_t lineSessionId = 0UL;
        uint32_t lineRepairId = 0UL;
        bool linked = false;
        if (!isValidSchema2Record(line, lineSessionId, linked, lineRepairId))
        {
            file.close();
            return WindingJournalQueryResult::ReadFailed;
        }

        const bool matches = sessionId != 0UL
            ? lineSessionId == sessionId
            : linked && lineRepairId == repairId;
        if (!matches) continue;

        if (matchedIndex < cursor)
        {
            if (matchedIndex == 0xFFFFFFFFUL)
            {
                file.close();
                return WindingJournalQueryResult::ReadFailed;
            }
            ++matchedIndex;
            continue;
        }

        if (count >= limit)
        {
            hasMore = true;
            break;
        }

        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
        if (matchedIndex == 0xFFFFFFFFUL)
        {
            file.close();
            return WindingJournalQueryResult::ReadFailed;
        }
        ++matchedIndex;
    }

    file.close();
    if (static_cast<uint32_t>(count) > 0xFFFFFFFFUL - cursor)
        return WindingJournalQueryResult::ReadFailed;
    nextCursor = cursor + static_cast<uint32_t>(count);
    return WindingJournalQueryResult::Ok;
}

bool WindingJournalQuery::findUnsigned(const String& line,
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

    if (line[cursor] == '0' &&
        cursor + 1 < line.length() &&
        isDigit(line[cursor + 1]))
    {
        return false;
    }

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

bool WindingJournalQuery::fieldIsNull(const String& line, const char* key)
{
    const String marker = String("\"") + key + F("\":null");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
        return false;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    return cursor < line.length() &&
           (line[cursor] == ',' || line[cursor] == '}');
}

bool WindingJournalQuery::isValidLegacySchema1Record(const String& line)
{
    uint32_t schemaVersion = 0UL;
    uint32_t runId = 0UL;
    uint32_t sessionId = 0UL;
    uint32_t completedRuns = 0UL;
    uint32_t uptimeMs = 0UL;
    if (!FlatJsonObjectValidator::valid(line) ||
        !findUnsigned(line, "schema_version", schemaVersion) ||
        schemaVersion != 1UL ||
        !findUnsigned(line, "run_id", runId) || runId == 0UL ||
        !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
        !findUnsigned(line, "completed_runs", completedRuns) ||
        completedRuns > 0xFFFFUL ||
        !findUnsigned(line, "uptime_ms", uptimeMs))
    {
        return false;
    }

    const String startedMarker = F("\"event\":\"RUN_STARTED\"");
    const String completedMarker = F("\"event\":\"RUN_COMPLETED\"");
    const int startedPosition = line.indexOf(startedMarker);
    const int completedPosition = line.indexOf(completedMarker);
    const bool started = startedPosition >= 0;
    const bool completed = completedPosition >= 0;
    if (started == completed ||
        (started && line.indexOf(startedMarker,
                                 startedPosition + startedMarker.length()) >= 0) ||
        (completed && line.indexOf(completedMarker,
                                   completedPosition + completedMarker.length()) >= 0) ||
        (started && completedRuns != 0UL) ||
        (completed && completedRuns == 0UL))
    {
        return false;
    }

    return true;
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
    if (!FlatJsonObjectValidator::valid(line) ||
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

    const String startedMarker = F("\"event\":\"RUN_STARTED\"");
    const String completedMarker = F("\"event\":\"RUN_COMPLETED\"");
    const int startedPosition = line.indexOf(startedMarker);
    const int completedPosition = line.indexOf(completedMarker);
    const bool started = startedPosition >= 0;
    const bool completed = completedPosition >= 0;
    if (started == completed ||
        (started && line.indexOf(startedMarker,
                                 startedPosition + startedMarker.length()) >= 0) ||
        (completed && line.indexOf(completedMarker,
                                   completedPosition + completedMarker.length()) >= 0) ||
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
