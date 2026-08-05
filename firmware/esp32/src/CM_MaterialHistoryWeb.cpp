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
        !parseUnsigned(m_server, "limit", 1UL, 100UL, parsedLimit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t count = 0U;
    if (!m_ledger.appendAdjustmentHistoryJson(response, materialId,
                                               static_cast<uint16_t>(parsedLimit), count))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"adjustment_history_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"material_id\":"); response += materialId;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}
}
