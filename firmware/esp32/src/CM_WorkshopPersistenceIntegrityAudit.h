#ifndef CM_WORKSHOP_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WORKSHOP_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class WorkshopPersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_WORKSHOP_PERSISTENCE_INTEGRITY_AUDIT_H
