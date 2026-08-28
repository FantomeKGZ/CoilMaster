#ifndef CM_CLIENT_REVISION_WEB_H
#define CM_CLIENT_REVISION_WEB_H

#include <WebServer.h>
#include "CM_RepairRegistry.h"

namespace CM
{
class ClientRevisionWeb
{
public:
    ClientRevisionWeb(WebServer& server, RepairRegistry& registry);
    void begin();

private:
    void handleUpdate();
    static bool parseUnsigned(WebServer& server, const char* name, uint32_t& value);

    WebServer& m_server;
    RepairRegistry& m_registry;
};
}

#endif // CM_CLIENT_REVISION_WEB_H
