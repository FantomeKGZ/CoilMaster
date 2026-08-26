#include "CM_RepairRegistryLookupWeb.h"

#include <SD.h>

namespace CM
{
namespace
{
void appendSearchPageMetadata(String& response,
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
    response += '}';
}

bool resolveMotor(RepairRegistry& registry,
                  WebServer& server,
                  uint32_t motorId)
{
    bool found = false;
    if (!registry.motorExists(motorId, found))
    {
        server.send(500, "application/json; charset=utf-8",
                    "{\"error\":\"motor_lookup_integrity_failed\"}");
        return false;
    }
    if (!found)
    {
        server.send(404, "application/json; charset=utf-8",
                    "{\"error\":\"motor_not_found\"}");
        return false;
    }
    return true;
}
}

RepairRegistryLookupWeb::RepairRegistryLookupWeb(WebServer& server,
                                                 RepairRegistry& registry)
    : m_server(server),
      m_registry(registry),
      m_windingVersions(SD),
      m_asReceivedSnapshots(SD),
      m_windingVersionsReady(false),
      m_asReceivedSnapshotsReady(false)
{
}

void RepairRegistryLookupWeb::begin()
{
    m_windingVersionsReady = m_windingVersions.begin();
    m_asReceivedSnapshotsReady = m_asReceivedSnapshots.begin();

    m_server.on("/api/clients/by-id", HTTP_GET,
                [this]() { handleClient(); });
    m_server.on("/api/motors/by-id", HTTP_GET,
                [this]() { handleMotor(); });
    m_server.on("/api/motors/repairs", HTTP_GET,
                [this]() { handleMotorRepairs(); });
    m_server.on("/api/motors/winding/latest", HTTP_GET,
                [this]() { handleMotorWindingLatest(); });
    m_server.on("/api/motors/winding/versions", HTTP_GET,
                [this]() { handleMotorWindingVersions(); });
    m_server.on("/api/repairs/by-id", HTTP_GET,
                [this]() { handleRepair(); });
    m_server.on("/api/repairs/as-received", HTTP_GET,
                [this]() { handleRepairAsReceived(); });
    m_server.on("/api/search/clients", HTTP_GET,
                [this]() { handleClientSearch(); });
    m_server.on("/api/search/repairs", HTTP_GET,
                [this]() { handleRepairSearch(); });
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
    if (!resolveMotor(m_registry, m_server, motorId)) return;

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

void RepairRegistryLookupWeb::handleMotorWindingLatest()
{
    if (!m_registry.ready() || !m_windingVersionsReady || !m_windingVersions.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_store_unavailable\"}");
        return;
    }
    uint32_t motorId = 0UL;
    if (!parseId("motor_id", motorId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_id\"}");
        return;
    }
    if (!resolveMotor(m_registry, m_server, motorId)) return;

    String item;
    item.reserve(960U);
    bool found = false;
    if (!m_windingVersions.appendLatestByMotorJson(item, motorId, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_integrity_failed\"}");
        return;
    }
    String response = F("{\"motor_id\":");
    response += motorId;
    response += F(",\"item\":");
    response += found ? item : F("null");
    response += F(",\"versioned\":");
    response += found ? F("true") : F("false");
    response += F(",\"legacy_motor_fallback_required\":");
    response += found ? F("false") : F("true");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryLookupWeb::handleMotorWindingVersions()
{
    if (!m_registry.ready() || !m_windingVersionsReady || !m_windingVersions.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_store_unavailable\"}");
        return;
    }
    uint32_t motorId = 0UL;
    if (!parseId("motor_id", motorId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_id\"}");
        return;
    }
    if (!resolveMotor(m_registry, m_server, motorId)) return;

    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 12UL;
    bool cursorPresent = false;
    bool limitPresent = false;
    if (!parseOptionalUnsigned("cursor", 0UL, 0xFFFFFFFFUL,
                               cursor, cursorPresent) ||
        !parseOptionalUnsigned("limit", 1UL, MotorWindingVersionStore::MaxPageSize,
                               parsedLimit, limitPresent))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }
    if (!limitPresent) parsedLimit = 12UL;
    const uint8_t limit = static_cast<uint8_t>(parsedLimit);

    String response = F("{\"items\":[");
    response.reserve(256U + static_cast<unsigned int>(limit) * 960U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_windingVersions.appendMotorPageJson(response, motorId, cursor, limit,
                                                count, nextCursor, hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_integrity_failed\"}");
        return;
    }
    response += F("],\"motor_id\":"); response += motorId;
    response += F(",\"count\":"); response += count;
    response += F(",\"limit\":"); response += limit;
    response += F(",\"cursor\":"); response += cursor;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor; else response += F("null");
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

void RepairRegistryLookupWeb::handleRepairAsReceived()
{
    if (!m_registry.ready() || !m_asReceivedSnapshotsReady || !m_asReceivedSnapshots.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_as_received_store_unavailable\"}");
        return;
    }
    uint32_t repairId = 0UL;
    if (!parseId("repair_id", repairId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    String repair;
    bool repairFound = false;
    if (!m_registry.appendRepairByIdJson(repair, repairId, repairFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_lookup_integrity_failed\"}");
        return;
    }
    if (!repairFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"repair_not_found\"}");
        return;
    }

    String item;
    item.reserve(1280U);
    bool found = false;
    if (!m_asReceivedSnapshots.appendByRepairIdJson(item, repairId, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_as_received_integrity_failed\"}");
        return;
    }
    String response = F("{\"repair_id\":"); response += repairId;
    response += F(",\"item\":"); response += found ? item : F("null");
    response += F(",\"snapshot_present\":"); response += found ? F("true") : F("false");
    response += F(",\"legacy_repair_without_snapshot\":");
    response += found ? F("false") : F("true");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryLookupWeb::handleClientSearch()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    String query = m_server.hasArg("q") ? m_server.arg("q") : String();
    query.trim();
    if (query.length() == 0U || query.length() > 120U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_search_query\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 6UL;
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
    if (!limitPresent) parsedLimit = 6UL;

    const uint8_t limit = static_cast<uint8_t>(parsedLimit);
    String response = F("{\"items\":[");
    response.reserve(256U + static_cast<unsigned int>(limit) * 320U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_registry.appendClientsSearchPageJson(response,
                                                query,
                                                cursor,
                                                limit,
                                                count,
                                                nextCursor,
                                                hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"client_search_failed\"}");
        return;
    }
    appendSearchPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryLookupWeb::handleRepairSearch()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    String query = m_server.hasArg("q") ? m_server.arg("q") : String();
    query.trim();
    if (query.length() == 0U || query.length() > 120U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_search_query\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 6UL;
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
    if (!limitPresent) parsedLimit = 6UL;

    const uint8_t limit = static_cast<uint8_t>(parsedLimit);
    String response = F("{\"items\":[");
    response.reserve(256U + static_cast<unsigned int>(limit) * 520U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_registry.appendRepairsSearchPageJson(response,
                                                query,
                                                cursor,
                                                limit,
                                                count,
                                                nextCursor,
                                                hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_search_failed\"}");
        return;
    }
    appendSearchPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
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
