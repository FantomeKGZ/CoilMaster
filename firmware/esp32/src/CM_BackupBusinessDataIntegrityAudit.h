#ifndef CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H
#define CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct BackupBusinessDataAuditMetrics
{
    uint32_t clientRecordCount = 0UL;
    uint32_t motorRecordCount = 0UL;
    uint32_t repairRecordCount = 0UL;
    uint32_t repairStatusRecordCount = 0UL;
    uint32_t pricingRecordCount = 0UL;
};

class BackupBusinessDataIntegrityAudit
{
public:
    // Workshop-only audit used by RepairRegistry::begin(). It validates clients,
    // motors, repairs and CLOSED status history but intentionally does not make
    // core registry readiness depend on the separate pricing ledger.
    static bool checkWorkshopRegistry(fs::FS& storage,
                                      BackupBusinessDataAuditMetrics& metrics);

    // Backup/deep-integrity adds pricing validation on top of the same
    // authoritative workshop checks.
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, BackupBusinessDataAuditMetrics& metrics);
};
}

#endif // CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H
