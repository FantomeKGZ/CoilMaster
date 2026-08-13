#ifndef CM_NETWORK_WEB_H
#define CM_NETWORK_WEB_H

#include <WebServer.h>

#include "CM_NetworkManager.h"
#include "CM_NetworkProfileStore.h"

namespace CM
{
class NetworkWeb
{
public:
    NetworkWeb(WebServer& server,
               NetworkProfileStore& store,
               NetworkManager& manager);
    void begin();

private:
    void handleProfiles();
    void handleSave();
    void handleDelete();
    void handleReconnect();

    WebServer& m_server;
    NetworkProfileStore& m_store;
    NetworkManager& m_manager;
};
}

#endif // CM_NETWORK_WEB_H
