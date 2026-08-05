#ifndef CM_CONDUCTOR_SETTINGS_WEB_H
#define CM_CONDUCTOR_SETTINGS_WEB_H

#include <Arduino.h>
#include <WebServer.h>
#include "CM_WarehouseStore.h"

namespace CM
{
class ConductorSettingsWeb
{
public:
    ConductorSettingsWeb(WebServer& server, WarehouseStore& store);
    void begin();

private:
    void handleGet();
    void handleSet();
    static bool parseUnsigned(WebServer& server,const char* name,uint32_t minValue,uint32_t maxValue,uint32_t& value);
    WebServer& m_server;
    WarehouseStore& m_store;
};
}
#endif
