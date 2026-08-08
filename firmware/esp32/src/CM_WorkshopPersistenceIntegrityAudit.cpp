#include "CM_WorkshopPersistenceIntegrityAudit.h"
#include "CM_RepairRegistry.h"

namespace CM
{
bool WorkshopPersistenceIntegrityAudit::check(fs::FS& storage)
{
    if (!storage.exists("/data/workshop")) return true;

    File directory = storage.open("/data/workshop", FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }
    directory.close();

    // RepairRegistry::begin() performs the authoritative persisted registry
    // validation. Because /data/workshop already exists here, its directory
    // setup path is a no-op and this audit performs no file creation/recovery.
    RepairRegistry registry(storage);
    return registry.begin();
}
}
