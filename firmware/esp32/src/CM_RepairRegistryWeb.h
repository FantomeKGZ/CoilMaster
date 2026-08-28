#ifndef CM_REPAIR_REGISTRY_WEB_H
#define CM_REPAIR_REGISTRY_WEB_H

#include <Arduino.h>
#include <WebServer.h>
#include "CM_ClientRevisionWeb.h"
#include "CM_RepairIntakeCoordinator.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class RepairRegistryWeb
{
public:
    RepairRegistryWeb(WebServer& server, RepairRegistry& registry);
    void begin();

private:
    void handleListClients();
    void handleCreateClient();
    void handleListMotors();
    void handleCreateMotor();
    void handleListRepairs();
    void handleCreateRepair();
    void handleRepairFinalization();
    void handleCloseRepair();
    static bool parseUnsigned(WebServer& server, const char* name,
                              uint32_t minimum, uint32_t maximum,
                              uint32_t& value);

    WebServer& m_server;
    RepairRegistry& m_registry;
    ClientRevisionWeb m_clientRevisionWeb{m_server, m_registry};
    RepairIntakeCoordinator* m_intake;
};
}

#endif
