#include "CM_MaterialUsageCorrectionWeb.h"
#include <SD.h>
#include "CM_MaterialUsageIdempotency.h"
#include "CM_RepairLifecycle.h"

namespace CM
{
MaterialUsageCorrectionWeb::MaterialUsageCorrectionWeb(WebServer& server, MaterialLedger& ledger)
    : m_server(server), m_ledger(ledger) {}

void MaterialUsageCorrectionWeb::begin()
{
    m_server.on("/api/materials/usage/corrections", HTTP_POST, [this]() { handlePost(); });
}

void MaterialUsageCorrectionWeb::handlePost()
{
    if (!m_ledger.ready()) { m_server.send(503,"application/json; charset=utf-8","{\"error\":\"materials_unavailable\",\"write_performed\":false}"); return; }
    uint32_t sourceUsageId=0UL, repairId=0UL, quantity=0UL;
    if (!parseUnsigned(m_server,"source_usage_id",1UL,0xFFFFFFFFUL,sourceUsageId) ||
        !parseUnsigned(m_server,"repair_id",1UL,0xFFFFFFFFUL,repairId) ||
        !parseUnsigned(m_server,"quantity_milli",1UL,0xFFFFFFFFUL,quantity) ||
        !m_server.hasArg("operation_id") || !MaterialUsageIdempotency::validOperationId(m_server.arg("operation_id")) ||
        !m_server.hasArg("timestamp") || m_server.arg("timestamp").length()<10U)
    { m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_usage_correction_fields\",\"write_performed\":false}"); return; }

    bool repairFound=false;
    if (!m_ledger.repairExists(repairId,repairFound)) { m_server.send(m_ledger.ready()?500:503,"application/json; charset=utf-8","{\"error\":\"repair_reference_read_failed\",\"write_performed\":false}"); return; }
    if (!repairFound) { m_server.send(404,"application/json; charset=utf-8","{\"error\":\"repair_not_found\",\"write_performed\":false}"); return; }
    bool repairOpen=false;
    if (!RepairLifecycle::isOpen(SD,repairId,repairOpen)) { m_server.send(503,"application/json; charset=utf-8","{\"error\":\"repair_lifecycle_unavailable\",\"write_performed\":false}"); return; }
    if (!repairOpen) { m_server.send(409,"application/json; charset=utf-8","{\"error\":\"repair_closed\",\"write_performed\":false}"); return; }

    MaterialUsageCorrection correction;
    correction.sourceUsageId=sourceUsageId; correction.repairId=repairId; correction.quantityMilli=quantity;
    correction.operationId=m_server.arg("operation_id"); correction.timestamp=m_server.arg("timestamp"); correction.comment=m_server.arg("comment");
    MaterialUsageCorrectionResult result;
    if (!m_ledger.correctUsage(correction,result)) { m_server.send(m_ledger.ready()?500:503,"application/json; charset=utf-8","{\"error\":\"usage_correction_integrity_failed\",\"write_performed\":false}"); return; }

    switch (result.status)
    {
    case MaterialUsageCorrectionStatus::SourceNotFound:
        m_server.send(404,"application/json; charset=utf-8","{\"error\":\"source_usage_not_found\",\"write_performed\":false}"); return;
    case MaterialUsageCorrectionStatus::SourceRepairMismatch:
        m_server.send(409,"application/json; charset=utf-8","{\"error\":\"source_usage_repair_mismatch\",\"write_performed\":false}"); return;
    case MaterialUsageCorrectionStatus::RunWireForbidden:
        m_server.send(409,"application/json; charset=utf-8","{\"error\":\"run_wire_correction_forbidden\",\"write_performed\":false,\"next_action\":\"USE_RUN_WIRE_WORKFLOW\"}"); return;
    case MaterialUsageCorrectionStatus::OperationIdConflict:
        m_server.send(409,"application/json; charset=utf-8","{\"error\":\"correction_operation_id_conflict\",\"write_performed\":false}"); return;
    case MaterialUsageCorrectionStatus::OverCorrection:
    {
        String r=F("{\"error\":\"usage_over_correction\",\"write_performed\":false,\"remaining_correctable_quantity_milli\":"); r+=result.remainingCorrectableQuantityMilli; r+=F(",\"reload_required\":true}");
        m_server.send(409,"application/json; charset=utf-8",r); return;
    }
    default: break;
    }

    const bool replay=result.status==MaterialUsageCorrectionStatus::DuplicateReplay;
    if (!replay && result.status!=MaterialUsageCorrectionStatus::Confirmed) { m_server.send(500,"application/json; charset=utf-8","{\"error\":\"usage_correction_state_invalid\",\"write_performed\":false}"); return; }
    String r; r.reserve(560U); r=F("{\"confirmed\":true,\"duplicate_replay\":"); r+=replay?F("true"):F("false");
    r+=F(",\"write_performed\":"); r+=replay?F("false"):F("true"); r+=F(",\"adjustment_id\":"); r+=result.adjustmentId;
    r+=F(",\"source_usage_id\":"); r+=result.sourceUsageId; r+=F(",\"repair_id\":"); r+=result.repairId; r+=F(",\"material_id\":"); r+=result.materialId;
    r+=F(",\"corrected_quantity_milli\":"); r+=result.correctedQuantityMilli; r+=F(",\"remaining_correctable_quantity_milli\":"); r+=result.remainingCorrectableQuantityMilli;
    r+=F(",\"stock_quantity_milli\":"); r+=result.stockQuantityMilli; r+=F(",\"unit_price_minor\":"); r+=result.unitPriceMinor; r+=F(",\"correction_line_cost_minor\":"); appendUInt64(r,result.correctionLineCostMinor);
    r+=F(",\"currency\":\""); r+=result.currency; r+=F("\",\"source_cost_policy\":\"PERSISTED_USAGE_SNAPSHOT\",\"source_usage_immutable\":true,\"correction_history\":\"APPEND_ONLY\"}");
    m_server.send(replay?200:201,"application/json; charset=utf-8",r);
}

bool MaterialUsageCorrectionWeb::parseUnsigned(WebServer& server,const char* name,uint32_t minimum,uint32_t maximum,uint32_t& value)
{
    value=0UL; if(!server.hasArg(name)) return false; const String s=server.arg(name); if(s.length()==0U||(s.length()>1U&&s[0]=='0')) return false;
    uint32_t parsed=0UL; for(size_t i=0U;i<s.length();++i){if(!isDigit(s[i]))return false;const uint8_t d=static_cast<uint8_t>(s[i]-'0');if(parsed>(0xFFFFFFFFUL-d)/10UL)return false;parsed=parsed*10UL+d;}
    if(parsed<minimum||parsed>maximum)return false; value=parsed; return true;
}

void MaterialUsageCorrectionWeb::appendUInt64(String& target,uint64_t value)
{
    char buffer[24]; snprintf(buffer,sizeof(buffer),"%llu",static_cast<unsigned long long>(value)); target+=buffer;
}
}
