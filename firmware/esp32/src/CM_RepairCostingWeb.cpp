#include "CM_RepairCostingWeb.h"

namespace CM
{
RepairCostingWeb::RepairCostingWeb(WebServer& server, RepairCosting& costing)
    : m_server(server), m_costing(costing) {}

void RepairCostingWeb::begin()
{
    m_server.on("/api/repairs/costing", HTTP_GET, [this]() { handleGet(); });
    m_server.on("/api/repairs/costing", HTTP_POST, [this]() { handleSavePricing(); });
    m_server.on("/api/repairs/pricing-history", HTTP_GET,
                [this]() { handlePricingHistory(); });
}

void RepairCostingWeb::handlePricingHistory()
{
    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    RepairCostSummary current;
    if (!m_costing.load(repairId, current))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"pricing_history_unavailable\"}");
        return;
    }

    String response;
    response.reserve(4480U);
    response = F("{\"repair_id\":");
    response += repairId;
    response += F(",\"items\":[");
    uint16_t count = 0U;
    PricingRevisionSnapshot latest;
    if (!m_costing.appendPricingRevisionsJson(response, repairId, count, latest))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"pricing_history_unavailable\"}");
        return;
    }
    const bool countMatches = count == current.pricingRevisionCount;
    const bool latestValuesMatch = (count == 0U && current.pricingRevisionCount == 0U &&
                                    current.labourCostMinor == 0ULL && current.clientPriceMinor == 0ULL) ||
                                   (count > 0U && latest.labourCostMinor == current.labourCostMinor &&
                                    latest.clientPriceMinor == current.clientPriceMinor &&
                                    latest.currency == current.currency);
    const bool latestTimestampMatches = (count == 0U && current.pricingUpdatedAt.length() == 0U) ||
                                        (count > 0U && latest.timestamp == current.pricingUpdatedAt);

    response += F("],\"count\":");
    response += count;
    response += F(",\"latest_revision\":");
    response += count;
    response += F(",\"current_pricing\":{\"labour_cost_minor\":");
    appendUInt64(response, current.labourCostMinor);
    response += F(",\"client_price_minor\":");
    appendUInt64(response, current.clientPriceMinor);
    response += F(",\"currency\":\"");
    response += current.currency;
    response += F("\",\"updated_at\":");
    if (current.pricingUpdatedAt.length() > 0U)
    {
        response += '"'; response += current.pricingUpdatedAt; response += '"';
    }
    else response += F("null");
    response += F("},\"history_count_matches_current\":");
    response += countMatches ? F("true") : F("false");
    response += F(",\"history_latest_values_match_current\":");
    response += latestValuesMatch ? F("true") : F("false");
    response += F(",\"history_latest_timestamp_matches_current\":");
    response += latestTimestampMatches ? F("true") : F("false");
    response += F(",\"history_matches_current_pricing\":");
    response += countMatches && latestValuesMatch && latestTimestampMatches ? F("true") : F("false");
    response += F(",\"current_pricing_source\":\"LATEST_APPEND_ONLY_REVISION\"");
    response += F(",\"source\":\"APPEND_ONLY_LOG\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairCostingWeb::handleGet()
{
    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json; charset=utf-8", "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    RepairCostSummary summary;
    if (!m_costing.load(repairId, summary))
    {
        m_server.send(503, "application/json; charset=utf-8", "{\"error\":\"costing_unavailable\"}");
        return;
    }

    const uint64_t materialWireCost = summary.copperWireCostMinor +
                                      summary.aluminiumWireCostMinor +
                                      summary.unknownWireCostMinor;
    const uint32_t materialWireLines = static_cast<uint32_t>(summary.copperWireLineCount) +
                                       static_cast<uint32_t>(summary.aluminiumWireLineCount) +
                                       static_cast<uint32_t>(summary.unknownWireLineCount);
    const uint64_t componentCost = summary.wireCostMinor + summary.materialCostMinor + summary.labourCostMinor;
    const uint64_t lossMinor = summary.totalCostMinor > summary.clientPriceMinor
                                   ? summary.totalCostMinor - summary.clientPriceMinor
                                   : 0ULL;
    const char* pricingStatus = "NOT_SET";
    if (summary.clientPriceMinor > 0ULL)
    {
        if (summary.clientPriceMinor > summary.totalCostMinor) pricingStatus = "PROFIT";
        else if (summary.clientPriceMinor == summary.totalCostMinor) pricingStatus = "BREAK_EVEN";
        else pricingStatus = "LOSS";
    }
    bool pricingDeltaMatches = summary.clientPriceMinor == 0ULL &&
                               summary.marginMinor == 0ULL && lossMinor == 0ULL;
    if (summary.clientPriceMinor > 0ULL)
    {
        if (summary.clientPriceMinor >= summary.totalCostMinor)
            pricingDeltaMatches = summary.clientPriceMinor == summary.totalCostMinor + summary.marginMinor && lossMinor == 0ULL;
        else
            pricingDeltaMatches = summary.totalCostMinor == summary.clientPriceMinor + lossMinor && summary.marginMinor == 0ULL;
    }

    String response;
    response.reserve(1450U);
    response = F("{\"repair_id\":"); response += repairId;
    response += F(",\"wire_line_count\":"); response += summary.wireLineCount;
    response += F(",\"material_line_count\":"); response += summary.materialLineCount;
    response += F(",\"wire_cost_minor\":"); appendUInt64(response, summary.wireCostMinor);
    response += F(",\"wire_materials\":{");
    response += F("\"CU\":{\"consumed_g\":"); response += summary.copperWireGrams;
    response += F(",\"line_count\":"); response += summary.copperWireLineCount;
    response += F(",\"cost_minor\":"); appendUInt64(response, summary.copperWireCostMinor);
    response += F("},\"AL\":{\"consumed_g\":"); response += summary.aluminiumWireGrams;
    response += F(",\"line_count\":"); response += summary.aluminiumWireLineCount;
    response += F(",\"cost_minor\":"); appendUInt64(response, summary.aluminiumWireCostMinor);
    response += F("},\"UNKNOWN\":{\"consumed_g\":"); response += summary.unknownWireGrams;
    response += F(",\"line_count\":"); response += summary.unknownWireLineCount;
    response += F(",\"cost_minor\":"); appendUInt64(response, summary.unknownWireCostMinor);
    response += F("}},\"wire_materials_source\":\"CONFIRMED_WRITE_OFFS\"");
    response += F(",\"wire_material_costs_match_wire_cost\":");
    response += materialWireCost == summary.wireCostMinor ? F("true") : F("false");
    response += F(",\"wire_material_counts_match_wire_count\":");
    response += materialWireLines == summary.wireLineCount ? F("true") : F("false");
    response += F(",\"material_cost_minor\":"); appendUInt64(response, summary.materialCostMinor);
    response += F(",\"labour_cost_minor\":"); appendUInt64(response, summary.labourCostMinor);
    response += F(",\"total_cost_minor\":"); appendUInt64(response, summary.totalCostMinor);
    response += F(",\"cost_components_match_total\":");
    response += componentCost == summary.totalCostMinor ? F("true") : F("false");
    response += F(",\"client_price_minor\":"); appendUInt64(response, summary.clientPriceMinor);
    response += F(",\"client_price_set\":");
    response += summary.clientPriceMinor > 0ULL ? F("true") : F("false");
    response += F(",\"margin_minor\":"); appendUInt64(response, summary.marginMinor);
    response += F(",\"loss_minor\":"); appendUInt64(response, lossMinor);
    response += F(",\"pricing_status\":\""); response += pricingStatus; response += '"';
    response += F(",\"pricing_status_source\":\"SERVER\"");
    response += F(",\"pricing_delta_matches_price\":");
    response += pricingDeltaMatches ? F("true") : F("false");
    response += F(",\"pricing_revision_count\":"); response += summary.pricingRevisionCount;
    response += F(",\"pricing_updated_at\":");
    if (summary.pricingUpdatedAt.length() > 0U)
    {
        response += '"'; response += summary.pricingUpdatedAt; response += '"';
    }
    else response += F("null");
    response += F(",\"pricing_revision_source\":\"APPEND_ONLY_LOG\"");
    response += F(",\"currency\":\""); response += summary.currency; response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairCostingWeb::handleSavePricing()
{
    uint32_t repairId = 0UL;
    uint32_t expectedRevision = 0UL;
    uint64_t labour = 0ULL;
    uint64_t clientPrice = 0ULL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !parseUnsigned(m_server, "expected_revision", 0UL, 0xFFFFUL, expectedRevision) ||
        !parseUnsigned64(m_server, "labour_cost_minor", labour) ||
        !parseUnsigned64(m_server, "client_price_minor", clientPrice))
    {
        m_server.send(400, "application/json; charset=utf-8", "{\"error\":\"invalid_costing_fields\"}");
        return;
    }

    RepairCostSummary current;
    if (!m_costing.load(repairId, current))
    {
        m_server.send(503, "application/json; charset=utf-8", "{\"error\":\"costing_unavailable\"}");
        return;
    }
    if (expectedRevision != current.pricingRevisionCount)
    {
        String conflict = F("{\"error\":\"pricing_revision_conflict\",\"expected_revision\":");
        conflict += expectedRevision;
        conflict += F(",\"current_revision\":");
        conflict += current.pricingRevisionCount;
        conflict += F(",\"reload_required\":true}");
        m_server.send(409, "application/json; charset=utf-8", conflict);
        return;
    }

    const String currency = m_server.hasArg("currency") ? m_server.arg("currency") : String("KGS");
    const String timestamp = m_server.arg("timestamp");
    if (!m_costing.savePricing(repairId, labour, clientPrice, currency, timestamp))
    {
        m_server.send(500, "application/json; charset=utf-8", "{\"error\":\"pricing_write_failed\"}");
        return;
    }

    String response = F("{\"saved\":true,\"previous_revision\":");
    response += current.pricingRevisionCount;
    response += F(",\"new_revision\":");
    response += static_cast<uint32_t>(current.pricingRevisionCount) + 1UL;
    response += F(",\"revision_source\":\"APPEND_ONLY_LOG\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool RepairCostingWeb::parseUnsigned(WebServer& server,const char* name,uint32_t minValue,uint32_t maxValue,uint32_t& value)
{
    if (!server.hasArg(name)) return false;
    const String source=server.arg(name); if(source.length()==0U)return false;
    for(size_t i=0U;i<source.length();++i)if(!isDigit(source[i]))return false;
    const unsigned long parsed=strtoul(source.c_str(),nullptr,10); if(parsed<minValue||parsed>maxValue)return false;
    value=static_cast<uint32_t>(parsed); return true;
}

bool RepairCostingWeb::parseUnsigned64(WebServer& server,const char* name,uint64_t& value)
{
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    for (size_t i = 0U; i < source.length(); ++i)
        if (!isDigit(source[i])) return false;
    value = strtoull(source.c_str(), nullptr, 10);
    return true;
}

void RepairCostingWeb::appendUInt64(String& target,uint64_t value)
{
    char buffer[24]; snprintf(buffer,sizeof(buffer),"%llu",static_cast<unsigned long long>(value)); target+=buffer;
}
}
