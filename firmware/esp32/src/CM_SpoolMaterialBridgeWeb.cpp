#include "CM_SpoolMaterialBridgeWeb.h"

namespace CM
{
void SpoolMaterialBridgeWeb::begin()
{
    m_server.on("/api/warehouse/spool-material-bridges", HTTP_GET,
                [this]() { handleLookup(); });
    m_server.on("/api/warehouse/spool-material-bridges", HTTP_POST,
                [this]() { handleCreate(); });
}

void SpoolMaterialBridgeWeb::handleLookup()
{
    if (!m_bridges.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"spool_material_bridge_unavailable\"}");
        return;
    }

    uint32_t spoolId = 0UL;
    if (!parseUnsigned(m_server, "spool_id", 1UL, 0xFFFFFFFFUL, spoolId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_spool_id\"}");
        return;
    }

    SpoolMaterialBridge bridge;
    bool found = false;
    if (!m_bridges.loadBySpool(spoolId, bridge, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"spool_bridge_read_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"spool_bridge_not_found\"}");
        return;
    }

    String response = F("{\"bridge_id\":");
    response += bridge.bridgeId;
    response += F(",\"spool_id\":");
    response += bridge.spoolId;
    response += F(",\"warehouse_item_id\":");
    response += bridge.warehouseItemId;
    response += F(",\"wire_type\":\"");
    response += bridge.wireType;
    response += F("\",\"diameter_hundredths_mm\":");
    response += bridge.diameterHundredthsMm;
    response += F(",\"linked_at\":\"");
    response += bridge.linkedAt;
    response += F("\",\"stock_mutated\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void SpoolMaterialBridgeWeb::handleCreate()
{
    if (!m_warehouse.ready() || !m_materials.ready() || !m_bridges.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"spool_material_bridge_unavailable\",\"write_performed\":false}");
        return;
    }

    uint32_t spoolId = 0UL;
    uint32_t warehouseItemId = 0UL;
    uint32_t confirm = 0UL;
    if (!parseUnsigned(m_server, "spool_id", 1UL, 0xFFFFFFFFUL, spoolId) ||
        !parseUnsigned(m_server, "warehouse_item_id", 1UL, 0xFFFFFFFFUL, warehouseItemId) ||
        !parseUnsigned(m_server, "confirm", 1UL, 1UL, confirm) || confirm != 1UL ||
        !m_server.hasArg("linked_at") ||
        m_server.arg("linked_at").length() < 10U ||
        m_server.arg("linked_at").length() > 32U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_bridge_fields\",\"required_confirmation\":1,\"write_performed\":false}");
        return;
    }

    ActiveWireSpoolIdentity spool;
    bool spoolFound = false;
    if (!m_warehouse.loadActiveSpoolIdentity(spoolId, spool, spoolFound))
    {
        m_server.send(m_warehouse.ready() ? 500 : 503,
                      "application/json; charset=utf-8",
                      m_warehouse.ready()
                          ? "{\"error\":\"spool_reference_read_failed\",\"write_performed\":false}"
                          : "{\"error\":\"warehouse_unavailable\",\"write_performed\":false}");
        return;
    }
    if (!spoolFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"active_spool_not_found\",\"write_performed\":false}");
        return;
    }

    MaterialItemState material;
    bool materialFound = false;
    if (!m_materials.loadActiveMaterialState(warehouseItemId, material, materialFound))
    {
        m_server.send(m_materials.ready() ? 500 : 503,
                      "application/json; charset=utf-8",
                      m_materials.ready()
                          ? "{\"error\":\"material_reference_read_failed\",\"write_performed\":false}"
                          : "{\"error\":\"materials_unavailable\",\"write_performed\":false}");
        return;
    }
    if (!materialFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"active_material_not_found\",\"write_performed\":false}");
        return;
    }
    if (material.unit != MaterialUnit::Gram || !material.hasWireMetadata)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"material_not_wire_catalog_item\",\"required_unit\":\"GRAM\",\"write_performed\":false}");
        return;
    }
    if (material.wireType != spool.wireType ||
        material.diameterHundredthsMm != spool.diameterHundredthsMm)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"spool_material_identity_mismatch\",\"write_performed\":false}");
        return;
    }

    SpoolMaterialBridge existing;
    bool bridgeFound = false;
    if (!m_bridges.loadBySpool(spoolId, existing, bridgeFound))
    {
        m_server.send(m_bridges.ready() ? 500 : 503,
                      "application/json; charset=utf-8",
                      m_bridges.ready()
                          ? "{\"error\":\"spool_bridge_read_failed\",\"write_performed\":false}"
                          : "{\"error\":\"spool_material_bridge_unavailable\",\"write_performed\":false}");
        return;
    }
    if (bridgeFound)
    {
        String response = F("{\"error\":\"spool_already_bridged\",\"write_performed\":false,\"bridge_id\":");
        response += existing.bridgeId;
        response += F(",\"warehouse_item_id\":");
        response += existing.warehouseItemId;
        response += '}';
        m_server.send(409, "application/json; charset=utf-8", response);
        return;
    }

    NewSpoolMaterialBridge bridge;
    bridge.spoolId = spool.spoolId;
    bridge.warehouseItemId = material.materialId;
    bridge.wireType = spool.wireType;
    bridge.diameterHundredthsMm = spool.diameterHundredthsMm;
    bridge.linkedAt = m_server.arg("linked_at");

    uint32_t bridgeId = 0UL;
    if (!m_bridges.append(bridge, bridgeId))
    {
        m_server.send(m_bridges.ready() ? 500 : 503,
                      "application/json; charset=utf-8",
                      m_bridges.ready()
                          ? "{\"error\":\"spool_bridge_write_failed\",\"write_performed\":false}"
                          : "{\"error\":\"spool_material_bridge_unavailable\",\"write_performed\":false}");
        return;
    }

    String response = F("{\"created\":true,\"write_performed\":true,\"stock_mutated\":false,\"bridge_id\":");
    response += bridgeId;
    response += F(",\"spool_id\":");
    response += bridge.spoolId;
    response += F(",\"warehouse_item_id\":");
    response += bridge.warehouseItemId;
    response += F(",\"wire_type\":\"");
    response += bridge.wireType;
    response += F("\",\"diameter_hundredths_mm\":");
    response += bridge.diameterHundredthsMm;
    response += F(",\"operator_confirmation_required\":true}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

bool SpoolMaterialBridgeWeb::parseUnsigned(WebServer& server,
                                           const char* name,
                                           uint32_t minimum,
                                           uint32_t maximum,
                                           uint32_t& value)
{
    value = 0UL;
    if (name == nullptr || !server.hasArg(name) || minimum > maximum) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}
}
