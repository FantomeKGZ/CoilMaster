#include "CM_MaterialLedgerWeb.h"
#include <SD.h>
#include "CM_RepairCosting.h"
#include "CM_RepairCostingWeb.h"

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
    repairCosting.begin();
    repairCostingWeb.begin();
}

void MaterialLedgerWeb::handleList()
{
    if (!m_ledger.ready()) { m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}"); return; }
    String response=F("{\"items\":["); response.reserve(4096U); uint16_t count=0U;
    if(!m_ledger.appendMaterialsJson(response,count)){m_server.send(500,"application/json; charset=utf-8","{\"error\":\"materials_read_failed\"}");return;}
    response+=F("],\"count\":");response+=count;response+=F(",\"currency_policy\":\"KGS_ONLY\",\"supported_currency\":\"KGS\"}");m_server.send(200,"application/json; charset=utf-8",response);
}

void MaterialLedgerWeb::handleCreate()
{
    if(!m_ledger.ready()){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");return;}
    MaterialUnit unit;uint32_t stock=0UL,price=0UL;String currency=m_server.hasArg("currency")?m_server.arg("currency"):String("KGS");currency.trim();currency.toUpperCase();
    if(currency!="KGS"){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"unsupported_currency\",\"supported_currency\":\"KGS\",\"currency_policy\":\"KGS_ONLY\"}");return;}
    if(!m_server.hasArg("name")||m_server.arg("name").length()==0U||!parseUnit(m_server.arg("unit"),unit)||!parseUnsigned(m_server,"stock_quantity_milli",0UL,0xFFFFFFFFUL,stock)||!parseUnsigned(m_server,"price_per_unit_minor",1UL,100000000UL,price)){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_material_fields\"}");return;}
    NewMaterial material;material.name=m_server.arg("name");material.unit=unit;material.stockQuantityMilli=stock;material.pricePerUnitMinor=price;material.currency=currency;material.comment=m_server.arg("comment");uint32_t materialId=0UL;
    if(!m_ledger.addMaterial(material,materialId)){m_server.send(500,"application/json; charset=utf-8","{\"error\":\"material_write_failed\"}");return;}
    String response=F("{\"created\":true,\"material_id\":");response+=materialId;response+=F(",\"currency\":\"KGS\",\"currency_policy\":\"KGS_ONLY\"}");m_server.send(201,"application/json; charset=utf-8",response);
}

void MaterialLedgerWeb::handleUsage()
{
    if(!m_ledger.ready()){m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\"}");return;}
    uint32_t repairId=0UL,materialId=0UL,quantity=0UL;
    if(!parseUnsigned(m_server,"repair_id",1UL,0xFFFFFFFFUL,repairId)||!parseUnsigned(m_server,"material_id",1UL,0xFFFFFFFFUL,materialId)||!parseUnsigned(m_server,"quantity_milli",1UL,0xFFFFFFFFUL,quantity)||!m_server.hasArg("timestamp")||m_server.arg("timestamp").length()<10U){m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_usage_fields\"}");return;}
    if(!m_ledger.repairExists(repairId)){m_server.send(404,"application/json; charset=utf-8","{\"error\":\"repair_not_found\"}");return;}
    RepairMaterialUsage usage;usage.repairId=repairId;usage.materialId=materialId;usage.quantityMilli=quantity;usage.timestamp=m_server.arg("timestamp");usage.comment=m_server.arg("comment");RepairMaterialUsageResult result;
    if(!m_ledger.confirmUsage(usage,result)){m_server.send(409,"application/json; charset=utf-8","{\"error\":\"usage_not_committed\"}");return;}
    char cost[24];snprintf(cost,sizeof(cost),"%llu",static_cast<unsigned long long>(result.lineCostMinor));String response=F("{\"confirmed\":true,\"usage_id\":");response+=result.usageId;response+=F(",\"repair_id\":");response+=repairId;response+=F(",\"material_id\":");response+=materialId;response+=F(",\"quantity_milli\":");response+=quantity;response+=F(",\"remaining_quantity_milli\":");response+=result.remainingQuantityMilli;response+=F(",\"unit_price_minor\":");response+=result.unitPriceMinor;response+=F(",\"line_cost_minor\":");response+=cost;response+=F(",\"currency\":\"");response+=result.currency;response+=F("\",\"repair_reference_validated\":true,\"line_cost_source\":\"PERSISTED_USAGE_SNAPSHOT\",\"value_rounding\":\"TRUNCATE_MINOR_UNIT\"}");m_server.send(201,"application/json; charset=utf-8",response);
}

bool MaterialLedgerWeb::parseUnsigned(WebServer& server,const char* name,uint32_t minimum,uint32_t maximum,uint32_t& value)
{
    if(!server.hasArg(name))return false;const String source=server.arg(name);if(source.length()==0U)return false;for(size_t i=0U;i<source.length();++i)if(!isDigit(source[i]))return false;const unsigned long parsed=strtoul(source.c_str(),nullptr,10);if(parsed<minimum||parsed>maximum)return false;value=static_cast<uint32_t>(parsed);return true;
}

bool MaterialLedgerWeb::parseUnit(const String& source,MaterialUnit& unit)
{
    String value=source;value.toUpperCase();if(value=="PIECE")unit=MaterialUnit::Piece;else if(value=="GRAM")unit=MaterialUnit::Gram;else if(value=="MILLILITRE"||value=="MILLILITER")unit=MaterialUnit::Millilitre;else if(value=="METRE"||value=="METER")unit=MaterialUnit::Metre;else if(value=="SQUARE_METRE"||value=="SQUARE_METER")unit=MaterialUnit::SquareMetre;else return false;return true;
}
}