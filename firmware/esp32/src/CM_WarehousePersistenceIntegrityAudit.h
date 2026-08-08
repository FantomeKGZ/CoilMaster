#ifndef CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H
#define CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class WarehousePersistenceIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_WAREHOUSE_PERSISTENCE_INTEGRITY_AUDIT_H
