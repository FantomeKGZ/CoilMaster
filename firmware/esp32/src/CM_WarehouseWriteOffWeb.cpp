#include "CM_WarehouseWeb.h"

namespace CM
{
void WarehouseWeb::beginWriteOff()
{
    m_server.on("/api/warehouse/write-offs", HTTP_POST,
                [this]() { handleConfirmWriteOff(); });
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

    String response;
    response.reserve(4480U);
    response = F("{\"repair_id\":");
    response += repairId;
    response += F(",\"items\":[");
    uint16_t count = 0U;
    uint32_t totalConsumed = 0UL;
    WriteOffMaterialTotals materialTotals;
    if (!m_store.appendConfirmedWriteOffsJson(response,
                                               repairId,
                                               count,
                                               totalConsumed,
                                               materialTotals))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"write_off_history_read_failed\"}");
        return;
    }

    const uint32_t materialConsumed = materialTotals.copperGrams +
                                      materialTotals.aluminiumGrams +
                                      materialTotals.unknownGrams;
    const uint32_t materialCount = static_cast<uint32_t>(materialTotals.copperCount) +
                                   static_cast<uint32_t>(materialTotals.aluminiumCount) +
                                   static_cast<uint32_t>(materialTotals.unknownCount);

    response += F("],\"count\":"); response += count;
    response += F(",\"total_consumed_g\":"); response += totalConsumed;
    response += F(",\"material_totals\":{");
    response += F("\"CU\":{\"consumed_g\":"); response += materialTotals.copperGrams;
    response += F(",\"count\":"); response += materialTotals.copperCount;
    response += F("},\"AL\":{\"consumed_g\":"); response += materialTotals.aluminiumGrams;
    response += F(",\"count\":"); response += materialTotals.aluminiumCount;
    response += F("},\"UNKNOWN\":{\"consumed_g\":"); response += materialTotals.unknownGrams;
    response += F(",\"count\":"); response += materialTotals.unknownCount;
    response += F("}},\"material_totals_source\":\"SERVER\"");
    response += F(",\"material_totals_match_total\":");
    response += materialConsumed == totalConsumed ? F("true") : F("false");
    response += F(",\"material_count_match_count\":");
    response += materialCount == static_cast<uint32_t>(count) ? F("true") : F("false");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void WarehouseWeb::handleConfirmWriteOff()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    uint32_t spoolId = 0UL;
    uint32_t repairId = 0UL;
    uint32_t before = 0UL;
    uint32_t after = 0UL;

    if (!parseUnsignedArg(m_server, "spool_id", 1UL, 0xFFFFFFFFUL, spoolId) ||
        !parseUnsignedArg(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !parseUnsignedArg(m_server, "weight_before_g", 1UL, 1000000UL, before) ||
        !parseUnsignedArg(m_server, "weight_after_g", 0UL, 999999UL, after))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_write_off_fields\"}");
        return;
    }

    if (after >= before)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"weight_after_must_be_lower\"}");
        return;
    }

    const String timestamp = m_server.arg("timestamp");
    if (timestamp.length() < 10U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"timestamp_required\"}");
        return;
    }

    ConfirmedSpoolWriteOff operation;
    operation.spoolId = spoolId;
    operation.repairId = repairId;
    operation.weightBeforeGrams = before;
    operation.weightAfterGrams = after;
    operation.timestamp = timestamp;
    operation.comment = m_server.arg("comment");

    SpoolWriteOffResult result;
    if (!m_store.confirmSpoolWriteOff(operation, result))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"write_off_not_committed\"}");
        return;
    }

    String response;
    response.reserve(320U);
    response = F("{\"confirmed\":true,\"movement_id\":");
    response += result.movementId;
    response += F(",\"spool_id\":"); response += spoolId;
    response += F(",\"repair_id\":"); response += repairId;
    response += F(",\"diameter_hundredths_mm\":");
    response += result.diameterHundredthsMm;
    response += F(",\"wire_type\":");
    if (result.wireType.length() > 0U)
    {
        response += '"'; response += result.wireType; response += '"';
    }
    else response += F("null");
    response += F(",\"legacy_unknown_material\":");
    response += result.wireType.length() > 0U ? F("false") : F("true");
    response += F(",\"consumed_g\":"); response += result.consumedGrams;
    response += F(",\"current_weight_g\":"); response += after;
    response += F(",\"price_per_kg_minor\":");
    response += result.pricePerKgMinor;
    response += F(",\"currency\":\""); response += result.currency;
    response += F("\"}");
    m_server.send(201, "application/json; charset=utf-8", response);
}
}
