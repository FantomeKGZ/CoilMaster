#ifndef CM_PERSISTENT_ID_INTEGRITY_AUDIT_H
#define CM_PERSISTENT_ID_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct PersistentIdIntegrityAuditMetrics
{
    uint32_t lastAllocatedId = 0UL;
};

class PersistentIdIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, PersistentIdIntegrityAuditMetrics& metrics);
};
}

#endif // CM_PERSISTENT_ID_INTEGRITY_AUDIT_H
