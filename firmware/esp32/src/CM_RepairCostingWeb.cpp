#include "CM_RepairCostingWeb.h"

namespace CM
{
RepairCostingWeb::RepairCostingWeb(WebServer& server, RepairCosting& costing)
    : m_server(server), m_costing(costing) {}

void RepairCostingWeb::begin()
{
    m_server.on("/api/repairs/costing", HTTP_GET, [this]() { handleGet(); });
    m_server.on("/api/repairs/costing", HTTP_POST, [this]() { handleSavePricing(); });
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
    const uint64_t componentTotal = summary.wireCostMinor +
                                    summary.materialCostMinor +
                                    summary.labourCostMinor;
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

    bool pricingDeltaMatches = false;
    if (summary.clientPriceMinor == 0ULL)
    {
        pricingDeltaMatches = summary.marginMinor == 0ULL && lossMinor == 0ULL;
    }
    else if (summary.clientPriceMinor > summary.totalCostMinor)
    {
        pricingDeltaMatches = summary.clientPriceMinor == summary.totalCostMinor + summary.marginMinor &&
                              lossMinor == 0ULL;
    }
    else if (summary.clientPriceMinor == summary.totalCostMinor)
    {
        pricingDeltaMatches = summary.marginMinor == 0ULL && lossMinor == 0ULL;
    }
    else
    {
        pricingDeltaMatches = summary.totalCostMinor == summary.clientPriceMinor + lossMinor &&
                              summary.marginMinor == 0ULL;
    }

    String response;
    response.reserve(1440U);
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
    response += componentTotal == summary.totalCostMinor ? F("true") : F("false");
    response += F(",\"client_price_minor\":"); appendUInt64(response, summary.clientPriceMinor);
    response += F(",\"client_price_set\":");
    response += summary.clientPriceMinor > 0ULL ? F("true") : F("false");
    response += F(",\"margin_minor\":"); appendUInt64(response, summary.marginMinor);
    response += F(",\"loss_minor\":"); appendUInt64(response, lossMinor);
    response += F(",\"pricing_status\":\""); response += pricingStatus; response += '"';
    response += F(",\"pricing_status_source\":\"SERVER\"");
    response += F(",\"pricing_delta_matches_price\":");
    response += pricingDeltaMatches ? F("true") : F("false");
    response += F(",\"currency\":\""); response += summary.currency; response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairCostingWeb::handleSavePricing()
{
    uint32_t repairId = 0UL;
    uint64_t labour = 0ULL, clientPrice = 0ULL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !parseUnsigned64(m_server, "labour_cost_minor", labour) ||
        !parseUnsigned64(m_server, "client_price_minor", clientPrice))
    {
        m_server.send(400, "application/json; charset=utf-8", "{\"error\":\"invalid_costing_fields\"}");
        return;
    }

    const String currency = m_server.hasArg("currency") ? m_server.arg("currency") : String("KGS");
    const String timestamp = m_server.arg("timestamp");
    if (!m_costing.savePricing(repairId, labour, clientPrice, currency, timestamp))
    {
        m_server.send(500, "application/json; charset=utf-8", "{\"error\":\"pricing_write_failed\"}");
        return;
    }
    m_server.send(200, "application/json; charset=utf-8", "{\"saved\":true}");
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
    if(!server.hasArg(name))return false; const String source=server.arg(name); if(source.length()==0U)return false;
    for(size_t i=0U;i<source.length();++i)if(!isDigit(source[i]))return false;
    value=strtoull(source.c_str(),nullptr,10); return true;
}

void RepairCostingWeb::appendUInt64(String& target,uint64_t value)
{
    char buffer[24]; snprintf(buffer,sizeof(buffer),"%llu",static_cast<unsigned long long>(value)); target+=buffer;
}
}
