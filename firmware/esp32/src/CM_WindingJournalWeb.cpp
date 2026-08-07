#include "CM_WindingJournalWeb.h"

#include <stdlib.h>

namespace CM
{
WindingJournalWeb::WindingJournalWeb(WebServer& server,
                                     WindingJournalQuery& query)
    : m_server(server), m_query(query)
{
}

void WindingJournalWeb::begin()
{
    m_server.on("/api/winding-history", HTTP_GET, [this]()
    {
        handleHistory();
    });
}

void WindingJournalWeb::handleHistory()
{
    const bool hasSessionId = m_server.hasArg("session_id");
    const bool hasRepairId = m_server.hasArg("repair_id");
    if (hasSessionId == hasRepairId)
    {
        m_server.send(400,
                      "application/json; charset=utf-8",
                      "{\"error\":\"exactly_one_filter_required\"}");
        return;
    }

    uint32_t sessionId = 0UL;
    uint32_t repairId = 0UL;
    if ((hasSessionId &&
         !parseCanonicalUint32(m_server.arg("session_id"), sessionId)) ||
        (hasRepairId &&
         !parseCanonicalUint32(m_server.arg("repair_id"), repairId)))
    {
        m_server.send(400,
                      "application/json; charset=utf-8",
                      "{\"error\":\"invalid_filter_id\"}");
        return;
    }

    uint16_t limit = DefaultLimit;
    if (m_server.hasArg("limit") &&
        !parseLimit(m_server.arg("limit"), limit))
    {
        m_server.send(400,
                      "application/json; charset=utf-8",
                      "{\"error\":\"invalid_limit\"}");
        return;
    }

    if (!m_query.isReady())
    {
        m_server.send(503,
                      "application/json; charset=utf-8",
                      "{\"error\":\"winding_history_unavailable\"}");
        return;
    }

    String events;
    events.reserve(512U);
    uint16_t count = 0U;
    const WindingJournalQueryResult result =
        m_query.appendHistoryJson(sessionId,
                                  repairId,
                                  limit,
                                  events,
                                  count);

    if (result == WindingJournalQueryResult::InvalidFilter)
    {
        m_server.send(400,
                      "application/json; charset=utf-8",
                      "{\"error\":\"invalid_history_filter\"}");
        return;
    }
    if (result == WindingJournalQueryResult::StorageUnavailable)
    {
        m_server.send(503,
                      "application/json; charset=utf-8",
                      "{\"error\":\"winding_history_unavailable\"}");
        return;
    }
    if (result == WindingJournalQueryResult::ReadFailed)
    {
        m_server.send(500,
                      "application/json; charset=utf-8",
                      "{\"error\":\"winding_history_read_failed\"}");
        return;
    }

    String response;
    response.reserve(events.length() + 160U);
    response = F("{\"filter\":{\"");
    if (sessionId != 0UL)
    {
        response += F("session_id\":");
        response += sessionId;
    }
    else
    {
        response += F("repair_id\":");
        response += repairId;
    }
    response += F("},\"limit\":");
    response += limit;
    response += F(",\"count\":");
    response += count;
    response += F(",\"events\":[");
    response += events;
    response += F("]}");

    m_server.send(200,
                  "application/json; charset=utf-8",
                  response);
}

bool WindingJournalWeb::parseCanonicalUint32(const String& text,
                                             uint32_t& value)
{
    value = 0UL;
    if (text.length() == 0U ||
        (text.length() > 1U && text[0] == '0'))
    {
        return false;
    }

    for (size_t index = 0U; index < text.length(); ++index)
    {
        if (!isDigit(text[index])) return false;
    }

    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(text.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0' || parsed == 0UL)
        return false;

    value = static_cast<uint32_t>(parsed);
    return String(value) == text;
}

bool WindingJournalWeb::parseLimit(const String& text,
                                   uint16_t& value)
{
    uint32_t parsed = 0UL;
    if (!parseCanonicalUint32(text, parsed) || parsed > MaximumLimit)
        return false;

    value = static_cast<uint16_t>(parsed);
    return true;
}
}
