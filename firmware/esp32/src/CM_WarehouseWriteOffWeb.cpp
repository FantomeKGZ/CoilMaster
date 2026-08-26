#include "CM_WarehouseWeb.h"

namespace CM
{
void WarehouseWeb::beginWriteOff()
{
    // Production operator mutation is atomic RUN_WIRE through
    // /api/material-requests/warehouse. Preserve this legacy URL for historical
    // GET/coverage only; old mutation clients must fail explicitly and cannot
    // bypass Material Request + Ledger + bridge transaction ownership.
    m_server.on("/api/warehouse/write-offs", HTTP_POST,
                [this]() {
                    m_server.send(410, "application/json; charset=utf-8",
                                  "{\"error\":\"legacy_writeoff_post_disabled\",\"write_performed\":false,\"replacement\":\"/api/material-requests/warehouse\"}");
                });
    m_server.on("/api/warehouse/write-offs", HTTP_GET,
                [this]() { handleListWriteOffs(); });
}

void WarehouseWeb::handleListWriteOffs()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsignedArg(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"repair_id_required\"}");
        return;
    }
    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 20UL;
    if ((m_server.hasArg("cursor") &&
         !parseUnsignedArg(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor)) ||
        (m_server.hasArg("limit") &&
         !parseUnsignedArg(m_server, "limit", 1UL,
                           WarehouseMaxListPageSize, parsedLimit)))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_paging_parameters\"}");
        return;
    }

    bool repairFound = false;
    if (!m_store.repairExists(repairId, repairFound))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repair_reference_read_failed\"}");
        return;
    }
    if (!repairFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"repair_not_found\"}");
        return;
    }

    String response;
    response.reserve(5400U);
    response = F("{\"repair_id\":");
    response += repairId;
    response += F(",\"items\":[");
    uint16_t count = 0U;
    uint16_t totalMatchingCount = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    uint32_t totalConsumed = 0UL;
    uint64_t totalValueMinor = 0ULL;
    WriteOffMaterialTotals materialTotals;
    if (!m_store.appendConfirmedWriteOffsPageJson(
            response, repairId, cursor, static_cast<uint8_t>(parsedLimit),
            count, totalMatchingCount, nextCursor, hasMore,
            totalConsumed, totalValueMinor, materialTotals))
    {
        if (!m_store.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"write_off_history_read_failed\"}");
        }
        return;
    }

    const uint32_t materialConsumed = materialTotals.copperGrams +
                                      materialTotals.aluminiumGrams +
                                      materialTotals.unknownGrams;
    const uint32_t materialCount = static_cast<uint32_t>(materialTotals.copperCount) +
                                   static_cast<uint32_t>(materialTotals.aluminiumCount) +
                                   static_cast<uint32_t>(materialTotals.unknownCount);
    const uint64_t materialValue = materialTotals.copperValueMinor +
                                   materialTotals.aluminiumValueMinor +
                                   materialTotals.unknownValueMinor;
    char valueBuffer[24];

    response += F("],\"count\":"); response += count;
    response += F(",\"returned_count\":"); response += count;
    response += F(",\"total_count\":"); response += totalMatchingCount;
    response += F(",\"cursor\":"); response += cursor;
    response += F(",\"limit\":"); response += parsedLimit;
    response += F(",\"max_page_size\":"); response += WarehouseMaxListPageSize;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor;
    else response += F("null");
    response += F(",\"total_consumed_g\":"); response += totalConsumed;
    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
             static_cast<unsigned long long>(totalValueMinor));
    response += F(",\"total_consumed_value_minor\":"); response += valueBuffer;
    response += F(",\"material_totals\":{");
    response += F("\"CU\":{\"consumed_g\":"); response += materialTotals.copperGrams;
    response += F(",\"count\":"); response += materialTotals.copperCount;
    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
             static_cast<unsigned long long>(materialTotals.copperValueMinor));
    response += F(",\"consumed_value_minor\":"); response += valueBuffer;
    response += F("},\"AL\":{\"consumed_g\":"); response += materialTotals.aluminiumGrams;
    response += F(",\"count\":"); response += materialTotals.aluminiumCount;
    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
             static_cast<unsigned long long>(materialTotals.aluminiumValueMinor));
    response += F(",\"consumed_value_minor\":"); response += valueBuffer;
    response += F("},\"UNKNOWN\":{\"consumed_g\":"); response += materialTotals.unknownGrams;
    response += F(",\"count\":"); response += materialTotals.unknownCount;
    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
             static_cast<unsigned long long>(materialTotals.unknownValueMinor));
    response += F(",\"consumed_value_minor\":"); response += valueBuffer;
    response += F("}},\"material_totals_source\":\"SERVER\"");
    response += F(",\"material_totals_match_total\":");
    response += materialConsumed == totalConsumed ? F("true") : F("false");
    response += F(",\"material_count_match_count\":");
    response += materialCount == static_cast<uint32_t>(totalMatchingCount) ? F("true") : F("false");
    response += F(",\"material_values_match_total\":");
    response += materialValue == totalValueMinor ? F("true") : F("false");
    response += F(",\"value_rounding\":\"NEAREST_MINOR_UNIT\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}
}