#ifndef CM_REPAIR_DELIVERY_INTEGRITY_AUDIT_H
#define CM_REPAIR_DELIVERY_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
class RepairDeliveryIntegrityAudit
{
public:
    // Backup-time read-only validation of immutable delivery evidence.
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, uint32_t& recordCount);
};
}

#endif // CM_REPAIR_DELIVERY_INTEGRITY_AUDIT_H
