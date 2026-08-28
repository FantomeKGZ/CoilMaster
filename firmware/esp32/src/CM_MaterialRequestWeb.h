#ifndef CM_MATERIAL_REQUEST_WEB_H
#define CM_MATERIAL_REQUEST_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_MaterialRequestMovementStore.h"
#include "CM_MaterialRequestStatusStore.h"
#include "CM_MaterialRequestStore.h"
#include "CM_MaterialRequestWarehouseCoordinator.h"
#include "CM_RepairRegistry.h"
#include "CM_RunWireIssueCoordinator.h"

namespace CM
{
class MaterialRequestWeb
{
public:
    MaterialRequestWeb(WebServer& server,
                       RepairRegistry& repairs,
                       MaterialRequestStore& requests,
                       MaterialRequestMovementStore& movements,
                       MaterialRequestStatusStore& statuses,
                       MaterialRequestWarehouseCoordinator& warehouse);
    MaterialRequestWeb(WebServer& server,
                       RepairRegistry& repairs,
                       MaterialRequestStore& requests,
                       MaterialRequestMovementStore& movements,
                       MaterialRequestStatusStore& statuses,
                       MaterialRequestWarehouseCoordinator& warehouse,
                       RunWireIssueCoordinator& runWire);

    void begin();

private:
    void handleCreate();
    void handleGetById();
    void handleListByRepair();
    void handleMovements();
    void handleStatus();
    void handleStatusBatch();
    void handleTransition();
    void handleWarehouseAction();

    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);
    static bool validTimestamp(const String& value);
    static void appendUint64(String& target, uint64_t value);

    WebServer& m_server;
    RepairRegistry& m_repairs;
    MaterialRequestStore& m_requests;
    MaterialRequestMovementStore& m_movements;
    MaterialRequestStatusStore& m_statuses;
    MaterialRequestWarehouseCoordinator& m_warehouse;
    RunWireIssueCoordinator* m_runWire;
};
}

#endif // CM_MATERIAL_REQUEST_WEB_H
