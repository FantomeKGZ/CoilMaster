#ifndef CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class WindingSessionPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_WINDING_SESSION_PERSISTENCE_INTEGRITY_AUDIT_H
