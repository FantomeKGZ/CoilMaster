#ifndef CM_RUN_WIRE_ACCOUNTING_INTEGRITY_AUDIT_H
#define CM_RUN_WIRE_ACCOUNTING_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct RunWireAccountingIntegrityMetrics
{
    uint32_t movementCount;
    uint32_t ledgerEvidenceCount;
    uint32_t warehouseEvidenceCount;

    RunWireAccountingIntegrityMetrics()
        : movementCount(0UL), ledgerEvidenceCount(0UL), warehouseEvidenceCount(0UL)
    {
    }
};

class RunWireAccountingIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, RunWireAccountingIntegrityMetrics& metrics);
};
}

#endif // CM_RUN_WIRE_ACCOUNTING_INTEGRITY_AUDIT_H
