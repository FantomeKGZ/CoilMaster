#ifndef CM_WINDING_JOURNAL_TRANSITION_AUDIT_H
#define CM_WINDING_JOURNAL_TRANSITION_AUDIT_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class WindingJournalTransitionAuditResult : uint8_t
{
    Ok = 0,
    StorageUnavailable = 1,
    ReadFailed = 2
};

class WindingJournalTransitionAudit
{
public:
    static WindingJournalTransitionAuditResult validate(fs::FS& storage);
    static WindingJournalTransitionAuditResult validate(fs::FS& storage,
                                                        uint32_t sessionId,
                                                        uint32_t runId,
                                                        bool& completed);
};
}

#endif // CM_WINDING_JOURNAL_TRANSITION_AUDIT_H
