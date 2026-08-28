#include "CM_MaterialLedgerWeb.h"
#include <SD.h>
#include "CM_RepairCosting.h"
#include "CM_RepairCostingWeb.h"
#include "CM_CashPaymentStore.h"
#include "CM_CashPaymentWeb.h"
#include "CM_RepairRegistry.h"
#include "CM_RepairLifecycle.h"
#include "CM_MaterialUsageIdempotency.h"

namespace CM
{
MaterialLedgerWeb::MaterialLedgerWeb(WebServer& server, MaterialLedger& ledger)
    : m_server(server), m_ledger(ledger) {}

void MaterialLedgerWeb::begin()
{
    m_server.on("/api/materials", HTTP_GET, [this]() { handleList(); });
    m_server.on("/api/materials", HTTP_POST, [this]() { handleCreate(); });
    m_server.on("/api/materials/adjust", HTTP_POST, [this]() { handleAdjust(); });
    m_server.on("/api/materials/adjustments", HTTP_GET,
                [this]() { handleAdjustmentHistory(); });
    m_server.on("/api/materials/usage", HTTP_GET,
                [this]() { handleUsageHistory(); });
    m_server.on("/api/materials/usage", HTTP_POST, [this]() { handleUsage(); });
    static RepairCosting repairCosting(SD);
    static RepairCostingWeb repairCostingWeb(m_server, repairCosting);
    static RepairRegistry cashRepairRegistry(SD);
    static CashPaymentStore cashPayments(SD);
    static CashPaymentWeb cashPaymentWeb(m_server, cashRepairRegistry, repairCosting, cashPayments);
    repairCosting.begin();
    repairCostingWeb.begin();
    if (cashRepairRegistry.begin() && cashPayments.begin()) cashPaymentWeb.begin();
}

void MaterialLedgerWeb::handleList()
{
    if (!m_ledger.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"materials_unavailable\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint32_t parsedLimit = 20UL;
    if ((m_server.hasArg("cursor") &&
         !parseUnsigned(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor)) ||
        (m_server.hasArg("limit") &&
         !parseUnsigned(m_server, "limit", 1UL,
                        MaterialLedger::MaxListPageSize, parsedLimit)))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String search = m_server.hasArg("search") ? m_server.arg("search") : String();
    search.trim();
    if (search.length() > MaterialLedger::MaxSearchLength)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"search_too_long\",\"max_search_length\":48}");
        return;
    }
    const uint8_t limit = static_cast<uint8_t>(parsedLimit);

    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(limit) * 384U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_ledger.appendMaterialsPageJson(response, search, cursor, limit, count,
                                          nextCursor, hasMore))
    {
        if (!m_ledger.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"materials_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"materials_read_failed\"}");
        return;
    }

    response += F("],\"count\":"); response += count;
    response += F(",\"limit\":"); response += static_cast<unsigned int>(limit);
    response += F(",\"cursor\":"); response += cursor;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":"); if (hasMore) response += nextCursor; else response += F("null");
    response += F(",\"search_active\":"); response += search.length() > 0U ? F("true") : F("false");
    response += F(",\"max_search_length\":"); response += static_cast<unsigned int>(MaterialLedger::MaxSearchLength);
    response += F(",\"max_page_size\":"); response += static_cast<unsigned int>(MaterialLedger::MaxListPageSize);
    response += F(",\"currency_policy\":\"KGS_ONLY\",\"supported_currency\":\"KGS\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MaterialLedgerWeb::handleCreate()
{
    if(!m_ledger.ready()){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");return;}
    MaterialUnit unit;uint32_t stock=0UL,price=0UL;String currency=m_server.hasArg("currency")?m_server.arg("currency"):String("KGS");currency.trim();currency.toUpperCase();
    if(currency!="KGS"){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"unsupported_currency\",\"supported_currency\":\"KGS\",\"currency_policy\":\"KGS_ONLY\"}");return;}
    if(!m_server.hasArg("name")||m_server.arg("name").length()==0U||!parseUnit(m_server.arg("unit"),unit)||!parseUnsigned(m_server,"stock_quantity_milli",0UL,0xFFFFFFFFUL,stock)||!parseUnsigned(m_server,"price_per_unit_minor",1UL,100000000UL,price)){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_material_fields\"}");return;}
    const bool hasWireType=m_server.hasArg("wire_type"),hasDiameter=m_server.hasArg("diameter_hundredths_mm");String wireType;uint32_t diameter=0UL;
    if(hasWireType!=hasDiameter){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"wire_metadata_pair_required\"}");return;}
    if(hasWireType){wireType=m_server.arg("wire_type");wireType.trim();wireType.toUpperCase();if(unit!=MaterialUnit::Gram||(wireType!="CU"&&wireType!="AL")||!parseUnsigned(m_server,"diameter_hundredths_mm",1UL,65535UL,diameter)){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_wire_metadata\",\"required_unit\":\"GRAM\"}");return;}}
    NewMaterial material;material.name=m_server.arg("name");material.unit=unit;material.stockQuantityMilli=stock;material.pricePerUnitMinor=price;material.currency=currency;material.comment=m_server.arg("comment");material.hasWireMetadata=hasWireType;if(hasWireType){material.wireType=wireType;material.diameterHundredthsMm=static_cast<uint16_t>(diameter);}uint32_t materialId=0UL;
    if(!m_ledger.addMaterial(material,materialId)){if(!m_ledger.ready())m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");else m_server.send(500,"application/json; charset=utf-8","{\"error\":\"material_write_failed\"}");return;}
    String response=F("{\"created\":true,\"material_id\":");response+=materialId;response+=F(",\"currency\":\"KGS\",\"currency_policy\":\"KGS_ONLY\",\"wire_metadata\":");response+=hasWireType?F("true"):F("false");response+='}';m_server.send(201,"application/json; charset=utf-8",response);
}

void MaterialLedgerWeb::handleUsage()
{
    if(!m_ledger.ready()){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");return;}
    uint32_t repairId=0UL,materialId=0UL,quantity=0UL,expectedStock=0UL,expectedPrice=0UL;
    if(!parseUnsigned(m_server,"repair_id",1UL,0xFFFFFFFFUL,repairId)||!parseUnsigned(m_server,"material_id",1UL,0xFFFFFFFFUL,materialId)||!parseUnsigned(m_server,"quantity_milli",1UL,0xFFFFFFFFUL,quantity)||!m_server.hasArg("timestamp")||m_server.arg("timestamp").length()<10U){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_usage_fields\"}");return;}
    if(!parseUnsigned(m_server,"expected_stock_quantity_milli",0UL,0xFFFFFFFFUL,expectedStock)||!parseUnsigned(m_server,"expected_price_per_unit_minor",1UL,100000000UL,expectedPrice)){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_material_preview\",\"write_performed\":false}");return;}
    if(!m_server.hasArg("operation_id")||!MaterialUsageIdempotency::validOperationId(m_server.arg("operation_id"))){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_operation_id\",\"write_performed\":false}");return;}
    const String operationId=m_server.arg("operation_id");
    const String usageComment=m_server.arg("comment");
    if(usageComment.indexOf(F("RWI_TX="))==0||usageComment.indexOf(F("MU_TX="))==0){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"reserved_usage_comment_prefix\",\"write_performed\":false}");return;}

    MaterialUsageReplay replay;
    if(!MaterialUsageIdempotency::lookup(SD,operationId,repairId,materialId,quantity,replay)){m_server.send(500,"application/json; charset=utf-8","{\"error\":\"usage_idempotency_read_failed\",\"write_performed\":false}");return;}
    if(replay.found)
    {
        if(!replay.payloadMatches){m_server.send(409,"application/json; charset=utf-8","{\"error\":\"operation_id_conflict\",\"write_performed\":false}");return;}
        MaterialItemState current;bool currentFound=false;
        if(!m_ledger.loadActiveMaterialState(materialId,current,currentFound)||!currentFound||current.currency!=replay.currency){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"material_state_unavailable_after_replay\",\"write_performed\":false}");return;}
        char cost[24];snprintf(cost,sizeof(cost),"%llu",static_cast<unsigned long long>(replay.lineCostMinor));
        String response=F("{\"confirmed\":true,\"duplicate_replay\":true,\"write_performed\":false,\"usage_id\":");response+=replay.usageId;response+=F(",\"repair_id\":");response+=repairId;response+=F(",\"material_id\":");response+=materialId;response+=F(",\"quantity_milli\":");response+=quantity;response+=F(",\"remaining_quantity_milli\":");response+=current.stockQuantityMilli;response+=F(",\"remaining_quantity_source\":\"CURRENT_AUTHORITATIVE_STATE\",\"unit_price_minor\":");response+=replay.unitPriceMinor;response+=F(",\"line_cost_minor\":");response+=cost;response+=F(",\"currency\":\"");response+=replay.currency;response+=F("\",\"repair_reference_validated\":true,\"material_currency_prevalidated\":true,\"line_cost_source\":\"PERSISTED_USAGE_SNAPSHOT\",\"value_rounding\":\"NEAREST_MINOR_UNIT\",\"historical_cost_policy\":\"USE_PERSISTED_LINE_COST\",\"currency_policy\":\"KGS_ONLY\"}");m_server.send(200,"application/json; charset=utf-8",response);return;
    }

    bool repairFound=false;
    if(!m_ledger.repairExists(repairId,repairFound)){if(!m_ledger.ready())m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");else m_server.send(500,"application/json; charset=utf-8","{\"error\":\"repair_reference_read_failed\"}");return;}
    if(!repairFound){m_server.send(404,"application/json; charset=utf-8","{\"error\":\"repair_not_found\"}");return;}
    bool repairOpen=false;if(!RepairLifecycle::isOpen(SD,repairId,repairOpen)){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"repair_lifecycle_unavailable\"}");return;}if(!repairOpen){m_server.send(409,"application/json; charset=utf-8","{\"error\":\"repair_closed\",\"write_performed\":false}");return;}

    MaterialItemState materialState;bool materialFound=false;
    if(!m_ledger.loadActiveMaterialState(materialId,materialState,materialFound)){if(!m_ledger.ready())m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");else m_server.send(500,"application/json; charset=utf-8","{\"error\":\"material_reference_read_failed\",\"write_performed\":false}");return;}
    if(!materialFound){m_server.send(404,"application/json; charset=utf-8","{\"error\":\"material_not_found\",\"write_performed\":false}");return;}
    if(materialState.currency!="KGS"){String error=F("{\"error\":\"unsupported_material_currency\",\"material_id\":");error+=materialId;error+=F(",\"material_currency\":\"");error+=materialState.currency;error+=F("\",\"supported_currency\":\"KGS\",\"currency_policy\":\"KGS_ONLY\",\"write_performed\":false}");m_server.send(409,"application/json; charset=utf-8",error);return;}
    if(quantity>materialState.stockQuantityMilli){String error=F("{\"error\":\"insufficient_stock\",\"write_performed\":false,\"material_id\":");error+=materialId;error+=F(",\"requested_quantity_milli\":");error+=quantity;error+=F(",\"available_quantity_milli\":");error+=materialState.stockQuantityMilli;error+=F(",\"current_price_per_unit_minor\":");error+=materialState.pricePerUnitMinor;error+='}';m_server.send(409,"application/json; charset=utf-8",error);return;}
    if(materialState.stockQuantityMilli!=expectedStock||materialState.pricePerUnitMinor!=expectedPrice){String error=F("{\"error\":\"stale_material_preview\",\"write_performed\":false,\"material_id\":");error+=materialId;error+=F(",\"expected_stock_quantity_milli\":");error+=expectedStock;error+=F(",\"current_stock_quantity_milli\":");error+=materialState.stockQuantityMilli;error+=F(",\"expected_price_per_unit_minor\":");error+=expectedPrice;error+=F(",\"current_price_per_unit_minor\":");error+=materialState.pricePerUnitMinor;error+='}';m_server.send(409,"application/json; charset=utf-8",error);return;}

    const String materialCurrency=materialState.currency;
    RepairMaterialUsage usage;usage.repairId=repairId;usage.materialId=materialId;usage.quantityMilli=quantity;usage.timestamp=m_server.arg("timestamp");usage.comment=MaterialUsageIdempotency::taggedComment(operationId,usageComment);RepairMaterialUsageResult result;
    if(!m_ledger.confirmUsage(usage,result)){if(!m_ledger.ready())m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");else m_server.send(409,"application/json; charset=utf-8","{\"error\":\"usage_not_committed\",\"write_performed\":false,\"next_action\":\"REFRESH_MATERIAL_AND_RETRY\"}");return;}
    const uint64_t expectedCost=(static_cast<uint64_t>(quantity)*static_cast<uint64_t>(result.unitPriceMinor)+500ULL)/1000ULL;const bool costMatches=result.lineCostMinor==expectedCost;const bool currencyMatches=result.currency=="KGS"&&result.currency==materialCurrency;
    char cost[24];snprintf(cost,sizeof(cost),"%llu",static_cast<unsigned long long>(result.lineCostMinor));String response=F("{\"confirmed\":true,\"duplicate_replay\":false,\"write_performed\":true,\"usage_id\":");response+=result.usageId;response+=F(",\"repair_id\":");response+=repairId;response+=F(",\"material_id\":");response+=materialId;response+=F(",\"quantity_milli\":");response+=quantity;response+=F(",\"remaining_quantity_milli\":");response+=result.remainingQuantityMilli;response+=F(",\"unit_price_minor\":");response+=result.unitPriceMinor;response+=F(",\"line_cost_minor\":");response+=cost;response+=F(",\"currency\":\"");response+=result.currency;response+=F("\",\"repair_reference_validated\":true,\"material_currency_prevalidated\":true,\"line_cost_source\":\"PERSISTED_USAGE_SNAPSHOT\",\"line_cost_formula\":\"ROUND(quantity_milli*unit_price_minor/1000)\",\"value_rounding\":\"NEAREST_MINOR_UNIT\",\"historical_cost_policy\":\"USE_PERSISTED_LINE_COST\",\"line_cost_matches_formula\":");response+=costMatches?F("true"):F("false");response+=F(",\"currency_matches_policy\":");response+=currencyMatches?F("true"):F("false");response+=F(",\"currency_policy\":\"KGS_ONLY\"}");m_server.send(201,"application/json; charset=utf-8",response);
}

bool MaterialLedgerWeb::parseUnsigned(WebServer& server,const char* name,uint32_t minimum,uint32_t maximum,uint32_t& value)
{
    value=0UL;if(!server.hasArg(name))return false;const String source=server.arg(name);if(source.length()==0U)return false;if(source.length()>1U&&source[0]=='0')return false;uint32_t parsed=0UL;for(size_t i=0U;i<source.length();++i){if(!isDigit(source[i]))return false;const uint8_t digit=static_cast<uint8_t>(source[i]-'0');if(parsed>(0xFFFFFFFFUL-digit)/10UL)return false;parsed=parsed*10UL+digit;}if(parsed<minimum||parsed>maximum)return false;value=parsed;return true;
}

bool MaterialLedgerWeb::parseUnit(const String& source,MaterialUnit& unit)
{
    String value=source;value.toUpperCase();if(value=="PIECE")unit=MaterialUnit::Piece;else if(value=="GRAM")unit=MaterialUnit::Gram;else if(value=="MILLILITRE"||value=="MILLILITER")unit=MaterialUnit::Millilitre;else if(value=="METRE"||value=="METER")unit=MaterialUnit::Metre;else if(value=="SQUARE_METRE"||value=="SQUARE_METER")unit=MaterialUnit::SquareMetre;else return false;return true;
}
}
