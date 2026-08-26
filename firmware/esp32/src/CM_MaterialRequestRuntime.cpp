#include "CM_MaterialRequestRuntime.h"

#include <SD.h>

#include "CM_MaterialLedger.h"
#include "CM_MaterialRequestMovementStore.h"
#include "CM_MaterialRequestStatusStore.h"
#include "CM_MaterialRequestStore.h"
#include "CM_MaterialRequestWarehouseCoordinator.h"
#include "CM_MaterialRequestWarehousePendingStore.h"
#include "CM_MaterialRequestWeb.h"

namespace CM
{
bool beginMaterialRequestRuntime(WebServer& server, RepairRegistry& repairs)
{
    static bool initialized = false;
    static bool registered = false;

    static MaterialLedger ledger(SD);
    static MaterialRequestStore requests(SD);
    static MaterialRequestMovementStore movements(SD);
    static MaterialRequestStatusStore statuses(SD);
    static MaterialRequestWarehousePendingStore pending(SD);
    static MaterialRequestWarehouseCoordinator warehouse(
        SD, ledger, requests, movements, statuses, pending);
    static MaterialRequestWeb web(
        server, repairs, requests, movements, statuses, warehouse);

    if (registered) return true;
    if (initialized) return false;
    initialized = true;

    if (!ledger.begin() ||
        !requests.begin() ||
        !movements.begin() ||
        !statuses.begin() ||
        !pending.begin() ||
        !warehouse.begin())
    {
        return false;
    }

    web.begin();
    registered = true;
    return true;
}
}
