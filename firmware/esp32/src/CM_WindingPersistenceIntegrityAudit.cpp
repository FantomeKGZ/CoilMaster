#include "CM_WindingPersistenceIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
bool WindingPersistenceIntegrityAudit::check(fs::FS& storage)
{
    WindingJournalQuery query(storage);
    if (!query.begin() || query.validateAll() != WindingJournalQueryResult::Ok)
        return false;

    return WindingJournalTransitionAudit::validate(storage) ==
           WindingJournalTransitionAuditResult::Ok;
}
}
