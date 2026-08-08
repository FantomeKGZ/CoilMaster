#include "CM_WindingSessionCompletionAudit.h"
#include "CM_WindingJournalQuery.h"

namespace CM
{
WindingSessionCompletionCheck WindingSessionCompletionAudit::check(fs::FS& storage,
                                                                  uint32_t sessionId)
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

        if (page.indexOf(F("\"event\":\"RUN_COMPLETED\"")) >= 0)
            completed = true;

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
