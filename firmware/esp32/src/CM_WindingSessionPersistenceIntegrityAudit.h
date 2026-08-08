#ifndef CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct WindingSessionPersistenceAuditMetrics
{
    uint32_t snapshotFileCount = 0UL;
    uint32_t stateFileCount = 0UL;
    uint32_t spoolSelectionFileCount = 0UL;
    bool byteTotalsAvailable = true;
    uint32_t snapshotTotalBytes = 0UL;
    uint32_t stateTotalBytes = 0UL;
    uint32_t spoolSelectionTotalBytes = 0UL;
};

class WindingSessionPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage,
                      WindingSessionPersistenceAuditMetrics& metrics);
};
}

#endif // CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H
