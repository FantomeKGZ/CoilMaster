#ifndef CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct WarehousePersistenceAuditMetrics
{
    uint32_t spoolRecordCount = 0UL;
    uint32_t priceRecordCount = 0UL;
    uint32_t spoolMaterialBridgeRecordCount = 0UL;
};

class WarehousePersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, WarehousePersistenceAuditMetrics& metrics);
};
}

#endif // CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H
