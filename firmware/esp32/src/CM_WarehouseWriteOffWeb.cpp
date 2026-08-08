#include "CM_WarehouseWeb.h"
#include <SD.h>
#include "CM_RepairLifecycle.h"
#include "CM_WindingSessionCompletionAudit.h"

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
    uint32_t totalConsumed = 0UL;
    uint64_t totalValueMinor = 0ULL;
    WriteOffMaterialTotals materialTotals;
    if (!m_store.appendConfirmedWriteOffsJson(response,
                                               repairId,
                                               count,
                                               totalConsumed,
                                               totalValueMinor,
                                               materialTotals))
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
    response += materialCount == static_cast<uint32_t>(count) ? F("true") : F("false");
    response += F(",\"material_values_match_total\":");
    response += materialValue == totalValueMinor ? F("true") : F("false");
    response += F(",\"value_rounding\":\"NEAREST_MINOR_UNIT\"}");
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

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_store.storage(), repairId, repairOpen))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_lifecycle_unavailable\"}");
        return;
    }
    if (!repairOpen)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"repair_closed\",\"write_performed\":false}");
        return;
    }

    if (after >= before)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"weight_after_must_be_lower\"}");
        return;
    }

    uint32_t sourceSessionId = 0UL;
    uint32_t sourceRunId = 0UL;
    const bool hasSourceSession = m_server.hasArg("source_session_id") &&
                                  m_server.arg("source_session_id").length() > 0U;
    const bool hasSourceRun = m_server.hasArg("source_run_id") &&
                              m_server.arg("source_run_id").length() > 0U;
    if (hasSourceSession != hasSourceRun)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"source_session_and_run_required_together\",\"write_performed\":false}");
        return;
    }

    if (hasSourceSession)
    {
        if (!parseUnsignedArg(m_server, "source_session_id", 1UL, 0xFFFFFFFFUL,
                              sourceSessionId) ||
            !parseUnsignedArg(m_server, "source_run_id", 1UL, 0xFFFFFFFFUL,
                              sourceRunId))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_source_session_or_run_id\",\"write_performed\":false}");
            return;
        }

        JobSpoolSelectionStore fallbackStore(m_store.storage());
        JobSpoolSelectionStore* selections = m_spoolSelections;
        if (selections == nullptr)
        {
            if (!fallbackStore.begin())
            {
                m_server.send(503, "application/json; charset=utf-8",
                              "{\"error\":\"job_spool_selection_store_unavailable\",\"write_performed\":false}");
                return;
            }
            selections = &fallbackStore;
        }
        if (!selections->isReady())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"job_spool_selection_store_unavailable\",\"write_performed\":false}");
            return;
        }

        JobSpoolSelection selection;
        bool found = false;
        if (!selections->load(sourceSessionId, selection, found))
        {
            if (!selections->isReady())
                m_server.send(503, "application/json; charset=utf-8",
                              "{\"error\":\"job_spool_selection_store_unavailable\",\"write_performed\":false}");
            else
                m_server.send(500, "application/json; charset=utf-8",
                              "{\"error\":\"job_spool_selection_read_failed\",\"write_performed\":false}");
            return;
        }
        if (!found)
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"source_session_spool_selection_not_found\",\"write_performed\":false}");
            return;
        }
        if (selection.repairId != repairId || selection.spoolId != spoolId)
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"source_session_spool_mismatch\",\"write_performed\":false}");
            return;
        }

        const WindingSessionCompletionCheck completion =
            WindingSessionCompletionAudit::check(m_store.storage(),
                                                 sourceSessionId,
                                                 sourceRunId);
        if (completion == WindingSessionCompletionCheck::StorageUnavailable)
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"winding_history_unavailable\",\"write_performed\":false}");
            return;
        }
        if (completion == WindingSessionCompletionCheck::IntegrityFailed)
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"winding_history_integrity_failed\",\"write_performed\":false}");
            return;
        }
        if (completion != WindingSessionCompletionCheck::Completed)
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"source_run_not_completed\",\"write_performed\":false}");
            return;
        }

        bool alreadyConfirmed = false;
        if (!m_store.confirmedWriteOffForSourceRun(sourceSessionId,
                                                   sourceRunId,
                                                   alreadyConfirmed))
        {
            if (!m_store.ready())
                m_server.send(503, "application/json; charset=utf-8",
                              "{\"error\":\"warehouse_unavailable\",\"write_performed\":false}");
            else
                m_server.send(500, "application/json; charset=utf-8",
                              "{\"error\":\"source_run_writeoff_lookup_failed\",\"write_performed\":false}");
            return;
        }
        if (alreadyConfirmed)
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"source_run_already_written_off\",\"write_performed\":false}");
            return;
        }
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
    operation.sourceSessionId = sourceSessionId;
    operation.sourceRunId = sourceRunId;
    operation.weightBeforeGrams = before;
    operation.weightAfterGrams = after;
    operation.timestamp = timestamp;
    operation.comment = m_server.arg("comment");

    SpoolWriteOffResult result;
    if (!m_store.confirmSpoolWriteOff(operation, result))
    {
        if (!m_store.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\",\"write_performed\":false}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"write_off_not_committed\"}");
        }
        return;
    }

    const uint64_t consumedValueMinor =
        (static_cast<uint64_t>(result.consumedGrams) * result.pricePerKgMinor + 500ULL) /
        1000ULL;
    char valueBuffer[24];
    snprintf(valueBuffer, sizeof(valueBuffer), "%llu",
             static_cast<unsigned long long>(consumedValueMinor));

    String response;
    response.reserve(480U);
    response = F("{\"confirmed\":true,\"movement_id\":");
    response += result.movementId;
    response += F(",\"spool_id\":"); response += spoolId;
    response += F(",\"repair_id\":"); response += repairId;
    response += F(",\"source_session_id\":");
    if (sourceSessionId != 0UL) response += sourceSessionId;
    else response += F("null");
    response += F(",\"source_run_id\":");
    if (sourceRunId != 0UL) response += sourceRunId;
    else response += F("null");
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
    response += F(",\"consumed_value_minor\":"); response += valueBuffer;
    response += F(",\"current_weight_g\":"); response += after;
    response += F(",\"price_per_kg_minor\":");
    response += result.pricePerKgMinor;
    response += F(",\"currency\":\""); response += result.currency;
    response += F("\",\"value_rounding\":\"NEAREST_MINOR_UNIT\",\"automatic_wire_writeoff_allowed\":false}");
    m_server.send(201, "application/json; charset=utf-8", response);
}
}
