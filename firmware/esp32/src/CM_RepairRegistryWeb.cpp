#include "CM_RepairRegistryWeb.h"
#include <SD.h>
#include "CM_RepairClosureGuard.h"
#include "CM_RepairFinalizationGuard.h"
#include "CM_WindingProgramParser.h"

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
    m_server.on("/api/repairs/close", HTTP_POST,
                [this]() { handleCloseRepair(); });
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
    if (!WindingProgramParser::valid(m_server.arg("coil_program")))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_coil_program\"}");
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

void RepairRegistryWeb::handleCloseRepair()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !m_server.hasArg("closed_at") || m_server.arg("closed_at").length() < 10U ||
        !m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_close_request\"}");
        return;
    }

    bool repairOpen = false;
    if (!m_registry.repairIsOpen(repairId, repairOpen))
    {
        if (!m_registry.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repair_close_state_read_failed\"}");
        return;
    }
    if (!repairOpen)
    {
        String response = F("{\"closed\":true,\"repair_id\":");
        response += repairId;
        response += F(",\"already_closed\":true,\"write_performed\":false,\"finalization_check_skipped\":true}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    bool closureAllowed = false;
    if (!RepairClosureGuard::canClose(SD, repairId, closureAllowed))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_closure_state_unavailable\"}");
        return;
    }
    if (!closureAllowed)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"repair_has_unfinished_winding_job\",\"write_performed\":false}");
        return;
    }

    const RepairFinalizationCheck finalization =
        RepairFinalizationGuard::check(SD, repairId);
    if (finalization == RepairFinalizationCheck::CostingStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_costing_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WindingStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_winding_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::CostingIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_costing_integrity_failed\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WindingIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_winding_integrity_failed\",\"write_performed\":false}");
        return;
    }
    if (finalization != RepairFinalizationCheck::Ready)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_unknown_failure\",\"write_performed\":false}");
        return;
    }

    bool alreadyClosed = false;
    if (!m_registry.closeRepair(repairId, m_server.arg("closed_at"), alreadyClosed))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"repair_not_closed\"}");
        }
        return;
    }

    String response = F("{\"closed\":true,\"repair_id\":");
    response += repairId;
    response += F(",\"already_closed\":");
    response += alreadyClosed ? F("true") : F("false");
    response += F(",\"write_performed\":true,\"finalization_integrity_verified\":true}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool RepairRegistryWeb::parseUnsigned(WebServer& server, const char* name,
                                      uint32_t minimum, uint32_t maximum,
                                      uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}
}
