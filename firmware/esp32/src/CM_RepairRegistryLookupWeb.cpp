#include "CM_RepairRegistryLookupWeb.h"

namespace CM
{
RepairRegistryLookupWeb::RepairRegistryLookupWeb(WebServer& server,
                                                 RepairRegistry& registry)
    : m_server(server), m_registry(registry)
{
}

void RepairRegistryLookupWeb::begin()
{
    m_server.on("/api/clients/by-id", HTTP_GET,
                [this]() { handleClient(); });
    m_server.on("/api/motors/by-id", HTTP_GET,
                [this]() { handleMotor(); });
    m_server.on("/api/repairs/by-id", HTTP_GET,
                [this]() { handleRepair(); });
}

void RepairRegistryLookupWeb::handleClient()
{
    uint32_t id = 0UL;
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!parseId("client_id", id))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_client_id\"}");
        return;
    }

    String response = F("{\"item\":");
    bool found = false;
    String item;
    item.reserve(384U);
    if (!m_registry.appendClientByIdJson(item, id, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"client_lookup_integrity_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"client_not_found\"}");
        return;
    }
    response += item;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryLookupWeb::handleMotor()
{
    uint32_t id = 0UL;
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!parseId("motor_id", id))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_id\"}");
        return;
    }

    String response = F("{\"item\":");
    bool found = false;
    String item;
    item.reserve(576U);
    if (!m_registry.appendMotorByIdJson(item, id, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_lookup_integrity_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }
    response += item;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryLookupWeb::handleRepair()
{
    uint32_t id = 0UL;
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!parseId("repair_id", id))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    String response = F("{\"item\":");
    bool found = false;
    String item;
    item.reserve(576U);
    if (!m_registry.appendRepairByIdJson(item, id, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_lookup_integrity_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"repair_not_found\"}");
        return;
    }
    response += item;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool RepairRegistryLookupWeb::parseId(const char* name, uint32_t& value) const
{
    value = 0UL;
    if (!m_server.hasArg(name)) return false;
    const String text = m_server.arg(name);
    if (text.length() == 0U || (text.length() > 1U && text[0] == '0'))
        return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < text.length(); ++index)
    {
        const char ch = text[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed == 0UL) return false;
    value = parsed;
    return true;
}
}
