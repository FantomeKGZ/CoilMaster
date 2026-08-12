#ifndef CM_REPAIR_REGISTRY_LOOKUP_WEB_H
#define CM_REPAIR_REGISTRY_LOOKUP_WEB_H

#include <WebServer.h>

#include "CM_RepairRegistry.h"

namespace CM
{
class RepairRegistryLookupWeb
{
public:
    RepairRegistryLookupWeb(WebServer& server, RepairRegistry& registry);
    void begin();

private:
    void handleClient();
    void handleMotor();
    void handleRepair();
    bool parseId(const char* name, uint32_t& value) const;

    WebServer& m_server;
    RepairRegistry& m_registry;
};
}

#endif
