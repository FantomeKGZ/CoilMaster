#ifndef CM_REPAIR_PRICING_INTEGRITY_AUDIT_H
#define CM_REPAIR_PRICING_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class RepairPricingIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_REPAIR_PRICING_INTEGRITY_AUDIT_H
