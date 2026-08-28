#ifndef CM_WAREHOUSE_WEB_H
#define CM_WAREHOUSE_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_JobSpoolSelectionStore.h"
#include "CM_WarehouseStore.h"

namespace CM
{
class WarehouseWeb
{
public:
    WarehouseWeb(WebServer& server, WarehouseStore& store)
        : m_server(server), m_store(store), m_spoolSelections(nullptr)
    {
    }
    WarehouseWeb(WebServer& server,
                 WarehouseStore& store,
                 JobSpoolSelectionStore* spoolSelections);
    WarehouseWeb(WebServer& server,
                 WarehouseStore& store,
                 JobSpoolSelectionStore& spoolSelections)
        : m_server(server), m_store(store), m_spoolSelections(&spoolSelections)
    {
    }

    void begin();
    void beginSpoolList();
    void beginWriteOff();

private:
    void handleSummary();
    void handleMaterialSummary();
    void handleCreateSpool();
    void handleListSpools();
    void handleGetActiveSpool();
    void handleAssignLegacySpoolMaterial();
    void handleListWriteOffs();
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
    JobSpoolSelectionStore* m_spoolSelections = nullptr;
};
}

#endif // CM_WAREHOUSE_WEB_H
