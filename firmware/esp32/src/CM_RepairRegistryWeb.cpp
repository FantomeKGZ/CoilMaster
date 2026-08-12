#include "CM_RepairRegistryWeb.h"
#include <SD.h>
#include "CM_BackupExportWeb.h"
#include "CM_RepairClosureGuard.h"
#include "CM_RepairFinalizationGuard.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
bool parseCanonicalUint32Text(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0'))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}

bool parsePaging(WebServer& server,
                 bool& requested,
                 uint32_t& cursor,
                 uint8_t& limit)
{
    requested = server.hasArg("cursor") || server.hasArg("limit");
    cursor = 0UL;
    limit = 20U;
    if (!requested) return true;

    if (server.hasArg("cursor") &&
        !parseCanonicalUint32Text(server.arg("cursor"), cursor))
    {
        return false;
    }

    if (server.hasArg("limit"))
    {
        uint32_t parsedLimit = 0UL;
        if (!parseCanonicalUint32Text(server.arg("limit"), parsedLimit) ||
            parsedLimit == 0UL || parsedLimit > RepairRegistry::MaxListPageSize)
        {
            return false;
        }
        limit = static_cast<uint8_t>(parsedLimit);
    }
    return true;
}

void appendPageMetadata(String& response,
                        uint16_t count,
                        uint8_t limit,
                        uint32_t cursor,
                        bool hasMore,
                        uint32_t nextCursor)
{
    response += F("],\"count\":");
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
    response += F(",\"max_page_size\":");
    response += RepairRegistry::MaxListPageSize;
    response += '}';
}
}

RepairRegistryWeb::RepairRegistryWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry) {}

void RepairRegistryWeb::begin()
{
    static BackupExportWeb backupExportWeb(m_server, SD);
    backupExportWeb.begin();
    m_server.on("/api/clients", HTTP_GET, [this]() { handleListClients(); });
    m_server.on("/api/clients", HTTP_POST, [this]() { handleCreateClient(); });
    m_server.on("/api/motors", HTTP_GET, [this]() { handleListMotors(); });
    m_server.on("/api/motors", HTTP_POST, [this]() { handleCreateMotor(); });
    m_server.on("/api/repairs", HTTP_GET, [this]() { handleListRepairs(); });
    m_server.on("/api/repairs", HTTP_POST, [this]() { handleCreateRepair(); });
    m_server.on("/api/repairs/finalization", HTTP_GET,
                [this]() { handleRepairFinalization(); });
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

    bool paged = false;
    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, paged, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    const String query = m_server.hasArg("phone") ? m_server.arg("phone") : String();

    if (paged)
    {
        response.reserve(384U + static_cast<unsigned int>(limit) * 320U);
        uint32_t nextCursor = 0UL;
        bool hasMore = false;
        if (!m_registry.appendClientsPageJson(response,
                                              query,
                                              cursor,
                                              limit,
                                              count,
                                              nextCursor,
                                              hasMore))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"clients_read_failed\"}");
            return;
        }
        appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    }
    else
    {
        response.reserve(4096U);
        if (!m_registry.appendClientsJson(response, query, count))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"clients_read_failed\"}");
            return;
        }
        response += F("],\"count\":"); response += count; response += '}';
    }

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

    bool paged = false;
    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, paged, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String query;
    if (m_server.hasArg("q")) query = m_server.arg("q");
    else if (m_server.hasArg("name")) query = m_server.arg("name");

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    if (paged)
    {
        response.reserve(384U + static_cast<unsigned int>(limit) * 560U);
        uint32_t nextCursor = 0UL;
        bool hasMore = false;
        if (!m_registry.appendMotorsPageJson(response,
                                             query,
                                             cursor,
                                             limit,
                                             count,
                                             nextCursor,
                                             hasMore))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motors_read_failed\"}");
            return;
        }
        appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    }
    else
    {
        response.reserve(4096U);
        if (!m_registry.appendMotorsJson(response, query, count))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motors_read_failed\"}");
            return;
        }
        response += F("],\"count\":"); response += count; response += '}';
    }

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

    bool paged = false;
    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, paged, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    if (paged)
    {
        response.reserve(384U + static_cast<unsigned int>(limit) * 520U);
        uint32_t nextCursor = 0UL;
        bool hasMore = false;
        if (!m_registry.appendRepairsPageJson(response,
                                              clientId,
                                              cursor,
                                              limit,
                                              count,
                                              nextCursor,
                                              hasMore))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repairs_read_failed\"}");
            return;
        }
        appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    }
    else
    {
        response.reserve(4096U);
        if (!m_registry.appendRepairsJson(response, clientId, count))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repairs_read_failed\"}");
            return;
        }
        response += F("],\"count\":"); response += count; response += '}';
    }

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

void RepairRegistryWeb::handleRepairFinalization()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_id\"}");
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
        String response = F("{\"repair_id\":");
        response += repairId;
        response += F(",\"status\":\"CLOSED\",\"ready_to_close\":false,\"already_closed\":true,\"reason\":\"already_closed\"}");
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
        String response = F("{\"repair_id\":");
        response += repairId;
        response += F(",\"status\":\"OPEN\",\"ready_to_close\":false,\"already_closed\":false,\"reason\":\"repair_has_unfinished_winding_job\"}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    const RepairFinalizationCheck finalization =
        RepairFinalizationGuard::check(SD, repairId);
    const char* reason = nullptr;
    switch (finalization)
    {
        case RepairFinalizationCheck::Ready:
            break;
        case RepairFinalizationCheck::CostingStorageUnavailable:
            reason = "repair_finalization_costing_storage_unavailable";
            break;
        case RepairFinalizationCheck::CostingIntegrityFailed:
            reason = "repair_finalization_costing_integrity_failed";
            break;
        case RepairFinalizationCheck::WindingStorageUnavailable:
            reason = "repair_finalization_winding_storage_unavailable";
            break;
        case RepairFinalizationCheck::WindingIntegrityFailed:
            reason = "repair_finalization_winding_integrity_failed";
            break;
        case RepairFinalizationCheck::WireWriteOffRequired:
            reason = "repair_finalization_wire_writeoff_required";
            break;
        case RepairFinalizationCheck::WireWriteOffStorageUnavailable:
            reason = "repair_finalization_wire_writeoff_storage_unavailable";
            break;
        case RepairFinalizationCheck::WireWriteOffIntegrityFailed:
            reason = "repair_finalization_wire_writeoff_integrity_failed";
            break;
        default:
            reason = "repair_finalization_unknown_failure";
            break;
    }

    String response = F("{\"repair_id\":");
    response += repairId;
    response += F(",\"status\":\"OPEN\",\"ready_to_close\":");
    response += finalization == RepairFinalizationCheck::Ready ? F("true") : F("false");
    response += F(",\"already_closed\":false,\"reason\":");
    if (reason == nullptr) response += F("null");
    else
    {
        response += '"';
        response += reason;
        response += '"';
    }
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
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
    if (finalization == RepairFinalizationCheck::WireWriteOffStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WireWriteOffRequired)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_required\",\"write_performed\":false}");
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
    if (finalization == RepairFinalizationCheck::WireWriteOffIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_integrity_failed\",\"write_performed\":false}");
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
