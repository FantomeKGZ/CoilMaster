#include "CM_MaterialRequestRuntime.h"

#include <SD.h>

#include "CM_MaterialLedger.h"
#include "CM_MaterialRequestMovementStore.h"
#include "CM_MaterialRequestStatusStore.h"
#include "CM_MaterialRequestStore.h"
#include "CM_MaterialRequestWarehouseCoordinator.h"
#include "CM_MaterialRequestWarehousePendingStore.h"
#include "CM_MaterialRequestWeb.h"
#include "CM_RunWireIssueCoordinator.h"
#include "CM_RunWireIssuePendingStore.h"
#include "CM_SpoolMaterialBridgeStore.h"
#include "CM_WarehouseStore.h"

namespace CM
{
bool beginMaterialRequestRuntime(WebServer& server, RepairRegistry& repairs)
{
    static bool initialized = false;
    static bool registered = false;

    // Runtime facades share the same authoritative SD files used by WarehouseWeb.
    // They do not create parallel catalogs. Recovery is synchronous and routes are
    // registered only after both the generic Material Request coordinator and the
    // exact-spool RUN_WIRE coordinator are ready.
    static MaterialLedger ledger(SD);
    static MaterialRequestStore requests(SD);
    static MaterialRequestMovementStore movements(SD);
    static MaterialRequestStatusStore statuses(SD);
    static MaterialRequestWarehousePendingStore pending(SD);
    static MaterialRequestWarehouseCoordinator warehouse(
        SD, ledger, requests, movements, statuses, pending);

    static RunWireIssuePendingStore runWirePending(SD);
    static SpoolMaterialBridgeStore spoolMaterialBridges(SD);
    static WarehouseStore physicalWarehouse(SD);
    static RunWireIssueCoordinator runWire(
        SD,
        ledger,
        requests,
        movements,
        statuses,
        runWirePending,
        spoolMaterialBridges,
        physicalWarehouse);

    static MaterialRequestWeb web(
        server, repairs, requests, movements, statuses, warehouse, runWire);

    if (registered) return true;
    if (initialized) return false;
    initialized = true;

    if (!ledger.begin() ||
        !requests.begin() ||
        !movements.begin() ||
        !statuses.begin() ||
        !pending.begin() ||
        !warehouse.begin() ||
        !runWirePending.begin() ||
        !spoolMaterialBridges.begin() ||
        !physicalWarehouse.begin() ||
        !runWire.begin())
    {
        return false;
    }

    web.begin();
    registered = true;
    return true;
}
}
