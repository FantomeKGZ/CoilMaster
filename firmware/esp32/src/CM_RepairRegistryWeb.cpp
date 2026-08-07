#include "CM_RepairRegistryWeb.h"

namespace CM
{
RepairRegistryWeb::RepairRegistryWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry) {}

void RepairRegistryWeb::begin()
{
    m_server.on("/api/clients", HTTP_GET, [this]() { handleListClients(); });
    m_server.on("/api/clients", HTTP_POST, [this]() { handleCreateClient(); });
    m_server.on("/api/motors", HTTP_GET, [this]() { handleListMotors(); });
    m_server.on("/api/motors", HTTP_POST, [this]() { handleCreateMotor(); });
    m_server.on("/api/repairs", HTTP_GET, [this]() { handleListRepairs(); });
    m_server.on("/api/repairs", HTTP_POST, [this]() { handleCreateRepair(); });
}

void RepairRegistryWeb::handleListClients()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t count = 0U;
    const String query = m_server.hasArg("phone") ? m_server.arg("phone") : String();
    if (!m_registry.appendClientsJson(response, query, count))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"clients_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count; response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateClient()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name") || !m_server.hasArg("phone") ||
        m_server.arg("name").length() == 0U ||
        RepairRegistry::normalizePhone(m_server.arg("phone")).length() < 7U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"name_and_valid_phone_required\"}");
        return;
    }
    NewClient client;
    client.name = m_server.arg("name");
    client.phone = m_server.arg("phone");
    client.comment = m_server.arg("comment");
    uint32_t clientId = 0UL;
    if (!m_registry.addClient(client, clientId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"client_write_failed\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"client_id\":");
    response += clientId; response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleListMotors()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t count = 0U;
    String query;
    if (m_server.hasArg("q")) query = m_server.arg("q");
    else if (m_server.hasArg("name")) query = m_server.arg("name");
    if (!m_registry.appendMotorsJson(response, query, count))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motors_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count; response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateMotor()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name") || m_server.arg("name").length() == 0U ||
        !m_server.hasArg("coil_program") ||
        m_server.arg("coil_program").length() == 0U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"name_and_coil_program_required\"}");
        return;
    }
    NewMotor motor;
    motor.name = m_server.arg("name");
    motor.model = m_server.arg("model");
    motor.manufacturer = m_server.arg("manufacturer");
    motor.tags = m_server.arg("tags");
    motor.coilProgram = m_server.arg("coil_program");
    motor.comment = m_server.arg("comment");
    uint32_t motorId = 0UL;
    if (!m_registry.addMotor(motor, motorId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motor_write_failed\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"motor_id\":");
    response += motorId; response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleListRepairs()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    uint32_t clientId = 0UL;
    if (m_server.hasArg("client_id") && m_server.arg("client_id").length() > 0U &&
        !parseUnsigned(m_server, "client_id", 1UL, 0xFFFFFFFFUL, clientId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_client_id\"}");
        return;
    }
    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t count = 0U;
    if (!m_registry.appendRepairsJson(response, clientId, count))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repairs_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count; response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateRepair()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    uint32_t clientId = 0UL;
    uint32_t motorId = 0UL;
    if (!parseUnsigned(m_server, "client_id", 1UL, 0xFFFFFFFFUL, clientId) ||
        !parseUnsigned(m_server, "motor_id", 1UL, 0xFFFFFFFFUL, motorId) ||
        !m_server.hasArg("received_at") || m_server.arg("received_at").length() < 10U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_fields\"}");
        return;
    }
    NewRepair repair;
    repair.clientId = clientId;
    repair.motorId = motorId;
    repair.receivedAt = m_server.arg("received_at");
    repair.complaint = m_server.arg("complaint");
    repair.comment = m_server.arg("comment");
    uint32_t repairId = 0UL;
    if (!m_registry.addRepair(repair, repairId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"repair_not_created\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"repair_id\":");
    response += repairId;
    response += F(",\"client_id\":"); response += clientId;
    response += F(",\"motor_id\":"); response += motorId;
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

bool RepairRegistryWeb::parseUnsigned(WebServer& server, const char* name,
                                      uint32_t minimum, uint32_t maximum,
                                      uint32_t& value)
{
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    for (size_t i = 0U; i < source.length(); ++i)
        if (!isDigit(source[i])) return false;
    const unsigned long parsed = strtoul(source.c_str(), nullptr, 10);
    if (parsed < minimum || parsed > maximum) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}
}
