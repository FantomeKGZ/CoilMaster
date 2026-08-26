#ifndef CM_SPOOL_MATERIAL_BRIDGE_WEB_H
#define CM_SPOOL_MATERIAL_BRIDGE_WEB_H

#include <WebServer.h>

#include "CM_MaterialLedger.h"
#include "CM_SpoolMaterialBridgeStore.h"
#include "CM_WarehouseStore.h"

namespace CM
{
// Explicit operator identity-link endpoint. POST appends bridge evidence and
// never mutates stock; GET exposes the immutable spool -> MaterialLedger link
// required by the explicit RUN_WIRE operator flow.
class SpoolMaterialBridgeWeb
{
public:
    SpoolMaterialBridgeWeb(WebServer& server,
                           WarehouseStore& warehouse,
                           MaterialLedger& materials,
                           SpoolMaterialBridgeStore& bridges)
        : m_server(server), m_warehouse(warehouse), m_materials(materials),
          m_bridges(bridges)
    {
    }

    void begin();

private:
    void handleCreate();
    void handleLookup();
    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);

    WebServer& m_server;
    WarehouseStore& m_warehouse;
    MaterialLedger& m_materials;
    SpoolMaterialBridgeStore& m_bridges;
};
}

#endif // CM_SPOOL_MATERIAL_BRIDGE_WEB_H
