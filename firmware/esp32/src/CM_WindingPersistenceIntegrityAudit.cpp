#include "CM_WindingPersistenceIntegrityAudit.h"
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
    return WindingJournalTransitionAudit::validate(storage, recordCount) ==
           WindingJournalTransitionAuditResult::Ok;
}
}
