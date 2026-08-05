#ifndef CM_WAREHOUSE_WEB_H
#define CM_WAREHOUSE_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_WarehouseStore.h"

namespace CM
{
class WarehouseWeb
{
public:
    WarehouseWeb(WebServer& server, WarehouseStore& store);

    void begin();

private:
    void handleSummary();
    static bool validMonth(const String& month);

    WebServer& m_server;
    WarehouseStore& m_store;
};
}

#endif // CM_WAREHOUSE_WEB_H
