#include "CM_RepairDeliveryWeb.h"

namespace CM
{
RepairDeliveryWeb::RepairDeliveryWeb(WebServer& server,
                                     RepairRegistry& repairs,
                                     RepairDeliveryStore& deliveries)
    : m_server(server), m_repairs(repairs), m_deliveries(deliveries)
{
}

void RepairDeliveryWeb::begin()
{
    m_server.on("/api/repairs/delivery", HTTP_GET,
                [this]() { handleGet(); });
    m_server.on("/api/repairs/delivery", HTTP_POST,
                [this]() { handleCreate(); });
}

void RepairDeliveryWeb::handleGet()
{
    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    RepairDeliveryState state;
    bool found = false;
    if (!m_deliveries.resolveByRepair(repairId, state, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"delivery_read_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"delivery_not_found\"}");
        return;
    }

    String response = F("{\"delivery_id\":"); response += state.deliveryId;
    response += F(",\"repair_id\":"); response += state.repairId;
    response += F(",\"client_id\":"); response += state.clientId;
    response += F(",\"motor_id\":"); response += state.motorId;
    response += F(",\"delivered_at\":\""); response += jsonEscape(state.deliveredAt); response += '"';
    if (state.comment.length() > 0U)
    {
        response += F(",\"comment\":\""); response += jsonEscape(state.comment); response += '"';
    }
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairDeliveryWeb::handleCreate()
{
    if (!m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !m_server.hasArg("delivered_at") || !validTimestamp(m_server.arg("delivered_at")))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_delivery_request\"}");
        return;
    }
    if (!m_repairs.ready() || !m_deliveries.ready())
    {
        m_server.send(503, "application/json", "{\"error\":\"delivery_store_unavailable\"}");
        return;
    }

    RepairIdentity identity;
    bool repairFound = false;
    if (!m_repairs.loadRepairIdentity(repairId, identity, repairFound))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_lookup_failed\"}");
        return;
    }
    if (!repairFound)
    {
        m_server.send(404, "application/json", "{\"error\":\"repair_not_found\"}");
        return;
    }

    bool repairOpen = false;
    if (!m_repairs.repairIsOpen(repairId, repairOpen))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_status_lookup_failed\"}");
        return;
    }
    if (repairOpen)
    {
        m_server.send(409, "application/json", "{\"error\":\"repair_must_be_closed_before_delivery\"}");
        return;
    }

    RepairDeliveryState existing;
    bool alreadyDelivered = false;
    if (!m_deliveries.resolveByRepair(repairId, existing, alreadyDelivered))
    {
        m_server.send(500, "application/json", "{\"error\":\"delivery_read_failed\"}");
        return;
    }
    if (alreadyDelivered)
    {
        m_server.send(409, "application/json", "{\"error\":\"repair_already_delivered\"}");
        return;
    }

    NewRepairDelivery delivery;
    delivery.repairId = repairId;
    delivery.clientId = identity.clientId;
    delivery.motorId = identity.motorId;
    delivery.deliveredAt = m_server.arg("delivered_at");
    delivery.comment = m_server.hasArg("comment") ? m_server.arg("comment") : String();

    uint32_t deliveryId = 0UL;
    if (!m_deliveries.append(delivery, deliveryId))
    {
        m_server.send(500, "application/json", "{\"error\":\"delivery_write_failed\"}");
        return;
    }

    String response = F("{\"delivered\":true,\"delivery_id\":"); response += deliveryId;
    response += F(",\"repair_id\":"); response += repairId;
    response += F(",\"client_id\":"); response += identity.clientId;
    response += F(",\"motor_id\":"); response += identity.motorId;
    response += F(",\"balance_gate_applied\":false}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

bool RepairDeliveryWeb::parseUnsigned(WebServer& server,
                                      const char* name,
                                      uint32_t minimum,
                                      uint32_t maximum,
                                      uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name)) return false;
    const String text = server.arg(name);
    if (text.length() == 0U || (text.length() > 1U && text[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < text.length(); ++i)
    {
        const char ch = text[i];
        if (ch < '0' || ch > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

bool RepairDeliveryWeb::validTimestamp(const String& value)
{
    return value.length() >= 10U && value.length() <= 32U;
}

String RepairDeliveryWeb::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"') { escaped += '\\'; escaped += ch; }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
}
