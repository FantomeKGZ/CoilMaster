#include "CM_MaterialLedgerWeb.h"

namespace CM
{
bool MaterialLedgerWeb::parseUnsignedValue(const String& source,
                                           uint32_t minimum,
                                           uint32_t maximum,
                                           uint32_t& value)
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
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

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

    const bool hasMaterialId = m_server.hasArg("material_id");
    const String materialIdSource = hasMaterialId
        ? m_server.arg("material_id") : String();
    if (hasMaterialId && materialIdSource.length() > 0U &&
        !parseUnsignedValue(materialIdSource, 1UL, 0xFFFFFFFFUL, materialId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_material_id\"}");
        return;
    }

    const bool hasLimit = m_server.hasArg("limit");
    const String limitSource = hasLimit ? m_server.arg("limit") : String();
    if (hasLimit && limitSource.length() > 0U &&
        !parseUnsignedValue(limitSource, 1UL,
                            MaterialLedger::MaxListPageSize, parsedLimit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_limit\"}");
        return;
    }

    uint32_t cursor = 0UL;
    if (m_server.hasArg("cursor"))
    {
        const String cursorSource = m_server.arg("cursor");
        if (!parseUnsignedValue(cursorSource, 0UL, 0xFFFFFFFFUL, cursor))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_cursor\"}");
            return;
        }
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
