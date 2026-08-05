#ifndef CM_REPAIR_REGISTRY_WEB_H
#define CM_REPAIR_REGISTRY_WEB_H

#include <Arduino.h>
#include <WebServer.h>
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
    void handleListRepairs();
    void handleCreateRepair();
    static bool parseUnsigned(WebServer& server, const char* name,
                              uint32_t minimum, uint32_t maximum,
                              uint32_t& value);

    WebServer& m_server;
    RepairRegistry& m_registry;
};
}

#endif
