#ifndef CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
struct MaterialPersistenceAuditMetrics
{
    uint32_t materialRecordCount = 0UL;
    uint32_t usageRecordCount = 0UL;
    uint32_t adjustmentRecordCount = 0UL;
};

class MaterialPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, MaterialPersistenceAuditMetrics& metrics);

    // Composite deep-audit helper: validates only the material domain and its
    // exact material/repair references. Callers must separately run their
    // authoritative workshop/pricing audit in the same fail-closed flow.
    static bool checkMaterialDomain(fs::FS& storage,
                                    MaterialPersistenceAuditMetrics& metrics);
};
}

#endif // CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H
