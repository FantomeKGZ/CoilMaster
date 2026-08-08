#include "CM_WorkshopPersistenceIntegrityAudit.h"
#include "CM_RepairRegistry.h"
#include "CM_WarehousePersistenceIntegrityAudit.h"
#include "CM_PersistentIdIntegrityAudit.h"
#include "CM_WindingSessionPersistenceIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
bool WorkshopPersistenceIntegrityAudit::check(fs::FS& storage)
{
    if (storage.exists("/data/workshop"))
    {
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
        if (!registry.begin()) return false;
    }

    if (!WarehousePersistenceIntegrityAudit::check(storage) ||
        !PersistentIdIntegrityAudit::check(storage) ||
        !WindingSessionPersistenceIntegrityAudit::check(storage))
    {
        return false;
    }

    WindingJournalQuery query(storage);
    if (!query.begin() || query.validateAll() != WindingJournalQueryResult::Ok)
        return false;

    return WindingJournalTransitionAudit::validate(storage) ==
           WindingJournalTransitionAuditResult::Ok;
}
}
