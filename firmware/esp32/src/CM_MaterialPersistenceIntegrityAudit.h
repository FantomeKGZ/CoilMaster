#ifndef CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class MaterialPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_MATERIAL_PERSISTENCE_INTEGRITY_AUDIT_H
