#include "CM_MaterialRequestWeb.h"

namespace CM
{
MaterialRequestWeb::MaterialRequestWeb(WebServer& server,
                                       RepairRegistry& repairs,
                                       MaterialRequestStore& requests,
                                       MaterialRequestMovementStore& movements,
                                       MaterialRequestStatusStore& statuses,
                                       MaterialRequestWarehouseCoordinator& warehouse)
    : m_server(server), m_repairs(repairs), m_requests(requests),
      m_movements(movements), m_statuses(statuses), m_warehouse(warehouse),
      m_runWire(nullptr)
{
}

MaterialRequestWeb::MaterialRequestWeb(WebServer& server,
                                       RepairRegistry& repairs,
                                       MaterialRequestStore& requests,
                                       MaterialRequestMovementStore& movements,
                                       MaterialRequestStatusStore& statuses,
                                       MaterialRequestWarehouseCoordinator& warehouse,
                                       RunWireIssueCoordinator& runWire)
    : m_server(server), m_repairs(repairs), m_requests(requests),
      m_movements(movements), m_statuses(statuses), m_warehouse(warehouse),
      m_runWire(&runWire)
{
}

void MaterialRequestWeb::begin()
{
    m_server.on("/api/material-requests", HTTP_POST, [this]() { handleCreate(); });
    m_server.on("/api/material-requests", HTTP_GET, [this]() { handleListByRepair(); });
    m_server.on("/api/material-requests/item", HTTP_GET, [this]() { handleGetById(); });
    m_server.on("/api/material-requests/movements", HTTP_GET, [this]() { handleMovements(); });
    m_server.on("/api/material-requests/status", HTTP_GET, [this]() { handleStatus(); });
    m_server.on("/api/material-requests/status-batch", HTTP_GET,
                [this]() { handleStatusBatch(); });
    m_server.on("/api/material-requests/status", HTTP_POST, [this]() { handleTransition(); });
    m_server.on("/api/material-requests/warehouse", HTTP_POST,
                [this]() { handleWarehouseAction(); });
}

void MaterialRequestWeb::handleCreate()
{
    if (!m_repairs.ready() || !m_requests.ready())
    {
        m_server.send(503, "application/json", "{\"error\":\"material_request_store_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_repair_id\"}");
        return;
    }
    if (!m_server.hasArg("created_at") || !validTimestamp(m_server.arg("created_at")))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_created_at\"}");
        return;
    }

    RepairIdentity identity;
    bool found = false;
    if (!m_repairs.loadRepairIdentity(repairId, identity, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_lookup_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"repair_not_found\"}");
        return;
    }
    bool open = false;
    if (!m_repairs.repairStatusIsOpen(repairId, open))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_status_lookup_failed\"}");
        return;
    }
    if (!open)
    {
        m_server.send(409, "application/json", "{\"error\":\"repair_closed\"}");
        return;
    }

    NewMaterialRequest request;
    request.repairId = repairId;
    request.clientId = identity.clientId;
    request.motorId = identity.motorId;
    request.createdAt = m_server.arg("created_at");
    request.comment = m_server.hasArg("comment") ? m_server.arg("comment") : String();

    uint32_t requestId = 0UL;
    if (!m_requests.append(request, requestId))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_create_failed\"}");
        return;
    }

    String response = F("{\"material_request_id\":"); response += requestId;
    response += F(",\"repair_id\":"); response += repairId;
    response += F(",\"client_id\":"); response += identity.clientId;
    response += F(",\"motor_id\":"); response += identity.motorId;
    response += F(",\"status\":\"DRAFT\"}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleGetById()
{
    uint32_t requestId = 0UL;
    if (!parseUnsigned(m_server, "material_request_id", 1UL, 0xFFFFFFFFUL, requestId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_id\"}");
        return;
    }
    String response;
    bool found = false;
    if (!m_requests.appendByIdJson(response, requestId, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_read_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"material_request_not_found\"}");
        return;
    }
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleListByRepair()
{
    uint32_t repairId = 0UL, cursor = 0UL, limitValue = 20UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_repair_id\"}");
        return;
    }
    if (m_server.hasArg("cursor") && m_server.arg("cursor").length() > 0U &&
        !parseUnsigned(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_cursor\"}");
        return;
    }
    if (m_server.hasArg("limit") && m_server.arg("limit").length() > 0U &&
        !parseUnsigned(m_server, "limit", 1UL, MaterialRequestStore::MaxPageSize, limitValue))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_requests.appendRepairPageJson(response, repairId, cursor,
                                         static_cast<uint8_t>(limitValue),
                                         count, nextCursor, hasMore))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_list_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":"); response += nextCursor;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleMovements()
{
    uint32_t requestId = 0UL, cursor = 0UL, limitValue = 20UL;
    if (!parseUnsigned(m_server, "material_request_id", 1UL, 0xFFFFFFFFUL, requestId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_id\"}");
        return;
    }
    if (m_server.hasArg("cursor") && m_server.arg("cursor").length() > 0U &&
        !parseUnsigned(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_cursor\"}");
        return;
    }
    if (m_server.hasArg("limit") && m_server.arg("limit").length() > 0U &&
        !parseUnsigned(m_server, "limit", 1UL, MaterialRequestMovementStore::MaxPageSize, limitValue))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_movements.appendRequestPageJson(response, requestId, cursor,
                                           static_cast<uint8_t>(limitValue),
                                           count, nextCursor, hasMore))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_movement_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":"); response += nextCursor;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleStatus()
{
    uint32_t requestId = 0UL;
    if (!parseUnsigned(m_server, "material_request_id", 1UL, 0xFFFFFFFFUL, requestId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_id\"}");
        return;
    }
    MaterialRequestStatusState state;
    bool found = false;
    if (!m_statuses.resolve(requestId, state, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_status_read_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"material_request_not_found\"}");
        return;
    }
    String response = F("{\"material_request_id\":"); response += requestId;
    response += F(",\"status\":\""); response += state.status;
    response += F("\",\"transition_count\":"); response += state.transitionCount;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleStatusBatch()
{
    if (!m_server.hasArg("ids"))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
        return;
    }
    const String text = m_server.arg("ids");
    if (text.length() == 0U || text.length() > 263U)
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
        return;
    }

    uint32_t ids[MaterialRequestStatusStore::MaxBatchSize];
    uint8_t count = 0U;
    size_t start = 0U;
    while (start < text.length())
    {
        if (count >= MaterialRequestStatusStore::MaxBatchSize)
        {
            m_server.send(400, "application/json", "{\"error\":\"material_request_status_batch_too_large\"}");
            return;
        }
        const int comma = text.indexOf(',', start);
        const size_t end = comma < 0 ? text.length() : static_cast<size_t>(comma);
        if (end <= start)
        {
            m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
            return;
        }
        const String token = text.substring(start, end);
        uint32_t parsed = 0UL;
        if (token.length() == 0U || (token.length() > 1U && token[0] == '0'))
        {
            m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
            return;
        }
        for (size_t i = 0U; i < token.length(); ++i)
        {
            if (token[i] < '0' || token[i] > '9')
            {
                m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
                return;
            }
            const uint8_t digit = static_cast<uint8_t>(token[i] - '0');
            if (parsed > (0xFFFFFFFFUL - digit) / 10UL)
            {
                m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
                return;
            }
            parsed = parsed * 10UL + digit;
        }
        if (parsed == 0UL)
        {
            m_server.send(400, "application/json", "{\"error\":\"invalid_material_request_ids\"}");
            return;
        }
        ids[count++] = parsed;
        if (comma < 0) break;
        start = end + 1U;
    }

    MaterialRequestStatusState states[MaterialRequestStatusStore::MaxBatchSize];
    bool found[MaterialRequestStatusStore::MaxBatchSize];
    if (!m_statuses.resolveBatch(ids, count, states, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"material_request_status_batch_read_failed\"}");
        return;
    }

    String response = F("{\"items\":[");
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (i > 0U) response += ',';
        response += F("{\"material_request_id\":"); response += ids[i];
        response += F(",\"found\":"); response += found[i] ? F("true") : F("false");
        if (found[i])
        {
            response += F(",\"status\":\""); response += states[i].status;
            response += F("\",\"transition_count\":"); response += states[i].transitionCount;
        }
        response += '}';
    }
    response += F("],\"count\":"); response += count;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleTransition()
{
    if (!m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }
    uint32_t requestId = 0UL;
    if (!parseUnsigned(m_server, "material_request_id", 1UL, 0xFFFFFFFFUL, requestId) ||
        !m_server.hasArg("target_status") ||
        !MaterialRequestStatusStore::validStatus(m_server.arg("target_status")) ||
        !m_server.hasArg("changed_at") || !validTimestamp(m_server.arg("changed_at")))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_status_transition_request\"}");
        return;
    }
    uint32_t transitionId = 0UL;
    if (!m_statuses.transition(requestId, m_server.arg("target_status"),
                               m_server.arg("changed_at"), transitionId))
    {
        m_server.send(409, "application/json", "{\"error\":\"status_transition_rejected\"}");
        return;
    }
    String response = F("{\"material_request_id\":"); response += requestId;
    response += F(",\"transition_id\":"); response += transitionId;
    response += F(",\"status\":\""); response += m_server.arg("target_status"); response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialRequestWeb::handleWarehouseAction()
{
    if (!m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }

    NewMaterialRequestMovement movement;
    if (!parseUnsigned(m_server, "material_request_id", 1UL, 0xFFFFFFFFUL,
                       movement.materialRequestId) ||
        !parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, movement.repairId) ||
        !parseUnsigned(m_server, "warehouse_item_id", 1UL, 0xFFFFFFFFUL,
                       movement.warehouseItemId) ||
        !parseUnsigned(m_server, "quantity_milli_units", 1UL, 0xFFFFFFFFUL,
                       movement.quantityMilliUnits) ||
        !m_server.hasArg("movement_kind") || !m_server.hasArg("source_kind") ||
        !m_server.hasArg("unit") || !m_server.hasArg("created_at") ||
        !validTimestamp(m_server.arg("created_at")))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_warehouse_operation\"}");
        return;
    }

    movement.movementKind = m_server.arg("movement_kind");
    movement.sourceKind = m_server.arg("source_kind");
    movement.unit = m_server.arg("unit");
    movement.createdAt = m_server.arg("created_at");
    movement.comment = m_server.hasArg("comment") ? m_server.arg("comment") : String();

    if (movement.sourceKind == "RUN_WIRE")
    {
        uint32_t diameter = 0UL;
        uint32_t spoolId = 0UL;
        if (m_runWire == nullptr || !m_runWire->ready())
        {
            m_server.send(503, "application/json", "{\"error\":\"run_wire_issue_unavailable\"}");
            return;
        }
        if (movement.movementKind != "ISSUE" || movement.unit != "KG" ||
            !parseUnsigned(m_server, "source_session_id", 1UL, 0xFFFFFFFFUL,
                           movement.sourceSessionId) ||
            !parseUnsigned(m_server, "source_run_id", 1UL, 0xFFFFFFFFUL,
                           movement.sourceRunId) ||
            !parseUnsigned(m_server, "spool_id", 1UL, 0xFFFFFFFFUL, spoolId) ||
            !parseUnsigned(m_server, "wire_diameter_hundredths_mm", 1UL, 500UL,
                           diameter) || !m_server.hasArg("material_class"))
        {
            m_server.send(400, "application/json", "{\"error\":\"invalid_run_wire_issue\"}");
            return;
        }
        movement.wireDiameterHundredthsMm = static_cast<uint16_t>(diameter);
        movement.materialClass = m_server.arg("material_class");

        RunWireIssueResult result;
        if (!m_runWire->execute(movement, spoolId, result))
        {
            m_server.send(409, "application/json", "{\"error\":\"run_wire_issue_rejected_or_recovery_required\"}");
            return;
        }

        String response = F("{\"movement_id\":");
        response += result.materialRequestMovementId;
        response += F(",\"transaction_ref\":\""); response += result.transactionRef;
        response += F("\",\"remaining_ledger_quantity_milli\":");
        response += result.remainingLedgerQuantityMilli;
        response += F(",\"spool_id\":"); response += spoolId;
        response += F(",\"spool_weight_after_g\":"); response += result.spoolWeightAfterGrams;
        response += F(",\"unit_cost_minor\":"); appendUint64(response, result.unitCostMinor);
        response += F(",\"cost_amount_minor\":"); appendUint64(response, result.costAmountMinor);
        response += F(",\"currency\":\""); response += result.currency;
        response += F("\"}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    if (!m_warehouse.ready())
    {
        m_server.send(503, "application/json", "{\"error\":\"material_request_warehouse_unavailable\"}");
        return;
    }

    String correctionDirection;
    if (movement.movementKind == "CORRECTION")
    {
        if (!m_server.hasArg("correction_direction"))
        {
            m_server.send(400, "application/json", "{\"error\":\"correction_direction_required\"}");
            return;
        }
        correctionDirection = m_server.arg("correction_direction");
    }

    MaterialRequestWarehouseResult result;
    if (!m_warehouse.execute(movement, correctionDirection, result))
    {
        m_server.send(409, "application/json", "{\"error\":\"warehouse_operation_rejected_or_recovery_required\"}");
        return;
    }

    String response = F("{\"movement_id\":"); response += result.movementId;
    response += F(",\"transaction_ref\":\""); response += result.transactionRef;
    response += F("\",\"remaining_ledger_quantity_milli\":");
    response += result.remainingLedgerQuantityMilli;
    response += F(",\"unit_cost_minor\":"); appendUint64(response, result.unitCostMinor);
    response += F(",\"cost_amount_minor\":"); appendUint64(response, result.costAmountMinor);
    response += F(",\"currency\":\""); response += result.currency;
    response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool MaterialRequestWeb::parseUnsigned(WebServer& server,
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
        if (text[i] < '0' || text[i] > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(text[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

bool MaterialRequestWeb::validTimestamp(const String& value)
{
    return value.length() >= 10U && value.length() <= 32U;
}

void MaterialRequestWeb::appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    target += buffer;
}
}