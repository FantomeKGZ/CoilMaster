#include "CM_WindingSessionCompletionAudit.h"
#include "CM_WindingJournalQuery.h"

namespace CM
{
namespace
{
bool pageContainsCompletedRun(const String& page, uint32_t runId)
{
    if (runId == 0UL) return page.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0;

    String runMarker = F("\"run_id\":");
    runMarker += runId;
    runMarker += ',';

    int cursor = 0;
    while (cursor < page.length())
    {
        const int start = page.indexOf('{', cursor);
        if (start < 0) break;
        const int end = page.indexOf('}', start + 1);
        if (end < 0) return false;
        const String record = page.substring(start, end + 1);
        if (record.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0 &&
            record.indexOf(runMarker) >= 0)
        {
            return true;
        }
        cursor = end + 1;
    }
    return false;
}
}

WindingSessionCompletionCheck WindingSessionCompletionAudit::check(fs::FS& storage,
                                                                  uint32_t sessionId)
{
    return check(storage, sessionId, 0UL);
}

WindingSessionCompletionCheck WindingSessionCompletionAudit::check(fs::FS& storage,
                                                                  uint32_t sessionId,
                                                                  uint32_t runId)
{
    if (sessionId == 0UL) return WindingSessionCompletionCheck::IntegrityFailed;

    WindingJournalQuery query(storage);
    if (!query.begin() || !query.isReady())
        return WindingSessionCompletionCheck::StorageUnavailable;

    uint32_t cursor = 0UL;
    bool completed = false;
    for (;;)
    {
        String page;
        page.reserve(4096U);
        uint16_t count = 0U;
        uint32_t nextCursor = cursor;
        bool hasMore = false;
        const WindingJournalQueryResult result =
            query.appendHistoryJson(sessionId,
                                    0UL,
                                    cursor,
                                    100U,
                                    page,
                                    count,
                                    nextCursor,
                                    hasMore);
        if (result == WindingJournalQueryResult::StorageUnavailable)
            return WindingSessionCompletionCheck::StorageUnavailable;
        if (result != WindingJournalQueryResult::Ok)
            return WindingSessionCompletionCheck::IntegrityFailed;

        if (pageContainsCompletedRun(page, runId)) completed = true;

        if (!hasMore) break;
        if (count == 0U || nextCursor <= cursor)
            return WindingSessionCompletionCheck::IntegrityFailed;
        cursor = nextCursor;
    }

    return completed
        ? WindingSessionCompletionCheck::Completed
        : WindingSessionCompletionCheck::NotCompleted;
}
}
