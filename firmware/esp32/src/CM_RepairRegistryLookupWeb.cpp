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
    m_server.on("/api/motors/repairs", HTTP_GET,
                [this]() { handleMotorRepairs(); });
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

void RepairRegistryLookupWeb::handleMotorRepairs()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t motorId = 0UL;
    if (!parseId("motor_id", motorId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_id\"}");
        return;
    }
    if (!m_registry.motorExists(motorId))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 20UL;
    bool cursorPresent = false;
    bool limitPresent = false;
    if (!parseOptionalUnsigned("cursor", 0UL, 0xFFFFFFFFUL,
                               cursor, cursorPresent) ||
        !parseOptionalUnsigned("limit", 1UL,
                               RepairRegistry::MaxListPageSize,
                               parsedLimit, limitPresent))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }
    if (!limitPresent) parsedLimit = 20UL;

    String statusFilter;
    if (m_server.hasArg("status") && m_server.arg("status").length() > 0U)
    {
        statusFilter = m_server.arg("status");
        if (statusFilter != "OPEN" && statusFilter != "CLOSED")
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_repair_status\"}");
            return;
        }
    }

    const uint8_t limit = static_cast<uint8_t>(parsedLimit);
    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(limit) * 520U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_registry.appendRepairsPageJson(response,
                                          0UL,
                                          motorId,
                                          statusFilter,
                                          cursor,
                                          limit,
                                          count,
                                          nextCursor,
                                          hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_repairs_read_failed\"}");
        return;
    }

    response += F("],\"motor_id\":");
    response += motorId;
    response += F(",\"count\":");
    response += count;
    response += F(",\"limit\":");
    response += limit;
    response += F(",\"cursor\":");
    response += cursor;
    response += F(",\"has_more\":");
    response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor;
    else response += F("null");
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

bool RepairRegistryLookupWeb::parseOptionalUnsigned(const char* name,
                                                    uint32_t minimum,
                                                    uint32_t maximum,
                                                    uint32_t& value,
                                                    bool& present) const
{
    present = m_server.hasArg(name) && m_server.arg(name).length() > 0U;
    if (!present) return true;
    const String text = m_server.arg(name);
    if (text.length() > 1U && text[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < text.length(); ++index)
    {
        const char ch = text[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
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
