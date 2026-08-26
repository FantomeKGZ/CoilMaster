#ifndef CM_CRM_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_CRM_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct CrmPersistenceAuditMetrics
{
    uint32_t windingVersionRecordCount = 0UL;
    uint32_t asReceivedRecordCount = 0UL;
    uint32_t materialRequestRecordCount = 0UL;
    uint32_t materialRequestMovementRecordCount = 0UL;
    uint32_t repairDeliveryRecordCount = 0UL;
};

class CrmPersistenceIntegrityAudit
{
public:
    // Read-only deep audit for backup/release integrity. It never repairs or
    // rewrites CRM journals; any malformed record/reference fails closed.
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, CrmPersistenceAuditMetrics& metrics);
};
}

#endif // CM_CRM_PERSISTENCE_INTEGRITY_AUDIT_H
