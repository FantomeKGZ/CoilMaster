#include "CM_WindingSessionCompletionAudit.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
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

    const WindingJournalQueryResult schemaAudit = query.validateAll();
    if (schemaAudit == WindingJournalQueryResult::StorageUnavailable)
        return WindingSessionCompletionCheck::StorageUnavailable;
    if (schemaAudit != WindingJournalQueryResult::Ok)
        return WindingSessionCompletionCheck::IntegrityFailed;

    bool completed = false;
    const WindingJournalTransitionAuditResult transitionAudit =
        WindingJournalTransitionAudit::validate(storage, sessionId, runId, completed);
    if (transitionAudit == WindingJournalTransitionAuditResult::StorageUnavailable)
        return WindingSessionCompletionCheck::StorageUnavailable;
    if (transitionAudit != WindingJournalTransitionAuditResult::Ok)
        return WindingSessionCompletionCheck::IntegrityFailed;

    return completed
        ? WindingSessionCompletionCheck::Completed
        : WindingSessionCompletionCheck::NotCompleted;
}
}
