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
    void beginSpoolList();
    void beginWriteOff();

private:
    void handleSummary();
    void handleMaterialSummary();
    void handleCreateSpool();
    void handleListSpools();
    void handleAssignLegacySpoolMaterial();
    void handleConfirmWriteOff();
    void handleGetPrice();
    void handleSetPrice();
    static bool validMonth(const String& month);
    static bool parseUnsignedArg(WebServer& server,
                                 const char* name,
                                 uint32_t minimum,
                                 uint32_t maximum,
                                 uint32_t& value);

    WebServer& m_server;
    WarehouseStore& m_store;
};
}

#endif // CM_WAREHOUSE_WEB_H
