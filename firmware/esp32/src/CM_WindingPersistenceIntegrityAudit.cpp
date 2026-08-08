#include "CM_WindingPersistenceIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
bool WindingPersistenceIntegrityAudit::check(fs::FS& storage)
{
    WindingJournalQuery query(storage);
    if (!query.begin()) return false;

    uint32_t cursor = 0UL;
    uint16_t pageCount = 0U;
    for (;;)
    {
        String ignoredJson;
        uint16_t count = 0U;
        uint32_t nextCursor = cursor;
        bool hasMore = false;
        const WindingJournalQueryResult result =
            query.appendHistoryJson(1UL,
                                    0UL,
                                    cursor,
                                    100U,
                                    ignoredJson,
                                    count,
                                    nextCursor,
                                    hasMore);
        if (result != WindingJournalQueryResult::Ok) return false;
        if (!hasMore) break;
        if (nextCursor <= cursor || pageCount == 0xFFFFU) return false;
        cursor = nextCursor;
        ++pageCount;
    }

    return WindingJournalTransitionAudit::validate(storage) ==
           WindingJournalTransitionAuditResult::Ok;
}
}
