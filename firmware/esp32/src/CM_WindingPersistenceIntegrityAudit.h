#ifndef CM_WINDING_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WINDING_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class WindingPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, uint32_t& recordCount);
};
}

#endif // CM_WINDING_PERSISTENCE_INTEGRITY_AUDIT_H
