#ifndef CM_SPOOL_MATERIAL_BRIDGE_INTEGRITY_AUDIT_H
#define CM_SPOOL_MATERIAL_BRIDGE_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
class SpoolMaterialBridgeIntegrityAudit
{
public:
    static bool check(fs::FS& storage, uint32_t& recordCount);
};
}

#endif // CM_SPOOL_MATERIAL_BRIDGE_INTEGRITY_AUDIT_H
