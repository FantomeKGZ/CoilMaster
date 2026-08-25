#include "CM_WindingJournalTransitionAudit.h"
#include "CM_WindingJournalQuery.h"

namespace CM
{
namespace
{
constexpr const char* JournalPath = "/data/winding-runs/events.ndjson";

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
        return false;

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
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;

    value = parsed;
    return true;
}

enum class AuditEvent : uint8_t
{
    Started = 0,
    Completed = 1
};

bool parseEvent(const String& line, AuditEvent& event)
{
    const String eventKey = F("\"event\":");
    const int eventKeyPos = line.indexOf(eventKey);
    if (eventKeyPos < 0 ||
        line.indexOf(eventKey, eventKeyPos + eventKey.length()) >= 0)
    {
        return false;
    }

    const String started = F("\"event\":\"RUN_STARTED\"");
    const String completed = F("\"event\":\"RUN_COMPLETED\"");
    const int startedPos = line.indexOf(started);
    const int completedPos = line.indexOf(completed);
    if ((startedPos >= 0) == (completedPos >= 0)) return false;

    const String& marker = startedPos >= 0 ? started : completed;
    const int position = startedPos >= 0 ? startedPos : completedPos;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;

    event = startedPos >= 0 ? AuditEvent::Started : AuditEvent::Completed;
    return true;
}

WindingJournalTransitionAuditResult validateInternal(fs::FS& storage,
                                                     uint32_t targetSessionId,
                                                     uint32_t targetRunId,
                                                     bool* completed)
{
    if (completed != nullptr) *completed = false;

    File root = storage.open("/", FILE_READ);
    if (!root) return WindingJournalTransitionAuditResult::StorageUnavailable;
    root.close();

    if (!storage.exists(JournalPath)) return WindingJournalTransitionAuditResult::Ok;
    File file = storage.open(JournalPath, FILE_READ);
    if (!file) return WindingJournalTransitionAuditResult::ReadFailed;

    uint32_t currentSessionId = 0UL;
    uint32_t activeRunId = 0UL;
    uint32_t highestRunId = 0UL;
    uint16_t completedRuns = 0U;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t sessionId = 0UL;
        uint32_t runId = 0UL;
        uint32_t persistedCompletedRuns = 0UL;
        AuditEvent event = AuditEvent::Started;

        if (!WindingJournalQuery::isValidRecord(line) ||
            !findUnsigned(line, "session_id", sessionId) || sessionId == 0UL ||
            !findUnsigned(line, "run_id", runId) || runId == 0UL ||
            !findUnsigned(line, "completed_runs", persistedCompletedRuns) ||
            persistedCompletedRuns > 0xFFFFUL || !parseEvent(line, event))
        {
            file.close();
            return WindingJournalTransitionAuditResult::ReadFailed;
        }

        if (currentSessionId == 0UL || sessionId > currentSessionId)
        {
            currentSessionId = sessionId;
            activeRunId = 0UL;
            highestRunId = 0UL;
            completedRuns = 0U;
        }
        else if (sessionId < currentSessionId)
        {
            file.close();
            return WindingJournalTransitionAuditResult::ReadFailed;
        }

        if (event == AuditEvent::Started)
        {
            if (persistedCompletedRuns != 0UL || activeRunId != 0UL || runId <= highestRunId)
            {
                file.close();
                return WindingJournalTransitionAuditResult::ReadFailed;
            }
            activeRunId = runId;
            highestRunId = runId;
            continue;
        }

        if (persistedCompletedRuns == 0UL || activeRunId == 0UL || runId != activeRunId ||
            completedRuns == 0xFFFFU ||
            persistedCompletedRuns != static_cast<uint32_t>(completedRuns) + 1UL)
        {
            file.close();
            return WindingJournalTransitionAuditResult::ReadFailed;
        }

        completedRuns = static_cast<uint16_t>(persistedCompletedRuns);
        activeRunId = 0UL;
        if (completed != nullptr && sessionId == targetSessionId &&
            (targetRunId == 0UL || runId == targetRunId))
        {
            *completed = true;
        }
    }

    file.close();
    return WindingJournalTransitionAuditResult::Ok;
}
}

WindingJournalTransitionAuditResult WindingJournalTransitionAudit::validate(fs::FS& storage)
{
    return validateInternal(storage, 0UL, 0UL, nullptr);
}

WindingJournalTransitionAuditResult WindingJournalTransitionAudit::validate(fs::FS& storage,
                                                                            uint32_t sessionId,
                                                                            uint32_t runId,
                                                                            bool& completed)
{
    completed = false;
    if (sessionId == 0UL)
        return WindingJournalTransitionAuditResult::ReadFailed;
    return validateInternal(storage, sessionId, runId, &completed);
}
}
