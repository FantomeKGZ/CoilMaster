#ifndef CM_PERSISTENT_ID_INTEGRITY_AUDIT_H
#define CM_PERSISTENT_ID_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class PersistentIdIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_PERSISTENT_ID_INTEGRITY_AUDIT_H
