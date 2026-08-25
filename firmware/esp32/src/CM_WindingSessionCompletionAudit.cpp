#include "CM_WindingSessionCompletionAudit.h"
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

    // TransitionAudit validates every record against the full winding-journal
    // schema while checking STARTED/COMPLETED ordering and exact completion
    // evidence. Do not restore the former query.validateAll() pre-pass here.
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
