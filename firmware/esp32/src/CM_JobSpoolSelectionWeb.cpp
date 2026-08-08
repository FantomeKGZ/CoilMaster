#include "CM_JobSpoolSelectionWeb.h"

namespace CM
{
JobSpoolSelectionWeb::JobSpoolSelectionWeb(WebServer& server,
                                           JobSpoolSelectionStore& store)
    : m_server(server), m_store(store) {}

void JobSpoolSelectionWeb::begin()
{
    m_server.on("/api/jobs/spool-selection", HTTP_GET,
                [this]() { handleGet(); });
}

void JobSpoolSelectionWeb::handleGet()
{
    uint32_t sessionId = 0UL;
    if (!m_server.hasArg("session_id") ||
        !parseCanonicalUint32(m_server.arg("session_id"), sessionId) ||
        sessionId == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_session_id\"}");
        return;
    }
    if (!m_store.isReady())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"job_spool_selection_store_unavailable\"}");
        return;
    }

    JobSpoolSelection selection;
    bool found = false;
    if (!m_store.load(sessionId, selection, found))
    {
        if (!m_store.isReady())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"job_spool_selection_store_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"job_spool_selection_read_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"job_spool_selection_not_found\"}");
        return;
    }

    String response = F("{\"session_id\":"); response += selection.sessionId;
    response += F(",\"job_id\":"); response += selection.jobId;
    response += F(",\"repair_id\":"); response += selection.repairId;
    response += F(",\"motor_id\":"); response += selection.motorId;
    response += F(",\"spool_id\":"); response += selection.spoolId;
    response += F(",\"diameter_hundredths_mm\":"); response += selection.diameterHundredthsMm;
    response += F(",\"weight_at_selection_g\":"); response += selection.weightAtSelectionGrams;
    response += F(",\"wire_type\":\""); response += selection.wireType;
    response += F("\",\"automatic_writeoff_allowed\":false}");
    m_server.sendHeader("Cache-Control", "no-store");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool JobSpoolSelectionWeb::parseCanonicalUint32(const String& source,
                                                uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}
}
