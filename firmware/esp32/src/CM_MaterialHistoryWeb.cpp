#include "CM_MaterialLedgerWeb.h"

namespace CM
{
void MaterialLedgerWeb::handleAdjustmentHistory()
{
    if (!m_ledger.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"materials_unavailable\"}");
        return;
    }

    uint32_t materialId = 0UL;
    uint32_t parsedLimit = 20UL;
    if (m_server.hasArg("material_id") && m_server.arg("material_id").length() > 0U &&
        !parseUnsigned(m_server, "material_id", 1UL, 0xFFFFFFFFUL, materialId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_material_id\"}");
        return;
    }
    if (m_server.hasArg("limit") && m_server.arg("limit").length() > 0U &&
        !parseUnsigned(m_server, "limit", 1UL, MaterialLedger::MaxListPageSize, parsedLimit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_limit\"}");
        return;
    }

    uint32_t cursor = 0UL;
    if (m_server.hasArg("cursor") &&
        !parseUnsigned(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor\"}");
        return;
    }

    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(parsedLimit) * 420U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_ledger.appendAdjustmentHistoryPageJson(
            response, materialId, cursor, static_cast<uint8_t>(parsedLimit),
            count, nextCursor, hasMore))
    {
        if (!m_ledger.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"materials_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"adjustment_history_read_failed\"}");
        }
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"material_id\":"); response += materialId;
    response += F(",\"limit\":"); response += parsedLimit;
    response += F(",\"cursor\":"); response += cursor;
    response += F(",\"has_more\":");
    response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor;
    else response += F("null");
    response += F(",\"max_page_size\":");
    response += static_cast<unsigned int>(MaterialLedger::MaxListPageSize);
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}
}
