#include "CM_WindingPersistenceIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
bool WindingPersistenceIntegrityAudit::check(fs::FS& storage)
{
    uint32_t ignoredRecordCount = 0UL;
    return check(storage, ignoredRecordCount);
}

bool WindingPersistenceIntegrityAudit::check(fs::FS& storage,
                                             uint32_t& recordCount)
{
    recordCount = 0UL;
    WindingJournalQuery query(storage);
    uint32_t validatedRecordCount = 0UL;
    if (!query.begin() ||
        query.validateAll(validatedRecordCount) != WindingJournalQueryResult::Ok)
    {
        return false;
    }

    if (WindingJournalTransitionAudit::validate(storage) !=
        WindingJournalTransitionAuditResult::Ok)
    {
        return false;
    }

    recordCount = validatedRecordCount;
    return true;
}
}
