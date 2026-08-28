#include "CM_ClientRevisionWeb.h"

namespace CM
{
ClientRevisionWeb::ClientRevisionWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry)
{
    begin();
}

void ClientRevisionWeb::begin()
{
    m_server.on("/api/clients/update", HTTP_POST, [this]() { handleUpdate(); });
}

void ClientRevisionWeb::handleUpdate()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\",\"write_performed\":false}");
        return;
    }

    uint32_t clientId = 0UL;
    if (!parseUnsigned(m_server, "client_id", clientId) ||
        !m_server.hasArg("name") || m_server.arg("name").length() == 0U ||
        !m_server.hasArg("phone") ||
        RepairRegistry::normalizePhone(m_server.arg("phone")).length() < 7U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_client_update_fields\",\"write_performed\":false}");
        return;
    }

    bool found = false;
    if (!m_registry.clientExists(clientId, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"client_reference_read_failed\",\"write_performed\":false}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"client_not_found\",\"write_performed\":false}");
        return;
    }

    NewClient client;
    client.name = m_server.arg("name");
    client.phone = m_server.arg("phone");
    client.comment = m_server.arg("comment");
    if (!m_registry.updateClient(clientId, client))
    {
        m_server.send(m_registry.ready() ? 500 : 503,
                      "application/json; charset=utf-8",
                      "{\"error\":\"client_update_failed\",\"write_performed\":false}");
        return;
    }

    String response = F("{\"updated\":true,\"write_performed\":true,\"client_id\":");
    response += clientId;
    response += F(",\"revision_history\":\"APPEND_ONLY\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool ClientRevisionWeb::parseUnsigned(WebServer& server, const char* name, uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U || (source.length() > 1U && source[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed == 0UL) return false;
    value = parsed;
    return true;
}
}
