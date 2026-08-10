#include "CM_AutonomousWindingWeb.h"

namespace CM
{
AutonomousWindingWeb::AutonomousWindingWeb(WebServer& server,
                                           AutonomousWindingArchive& archive,
                                           RepairRegistry& registry)
    : m_server(server), m_archive(archive), m_registry(registry)
{
}

void AutonomousWindingWeb::begin()
{
    m_server.on("/api/autonomous-windings", HTTP_GET,
                [this]() { handleList(); });
    m_server.on("/api/autonomous-windings/assign", HTTP_POST,
                [this]() { handleAssign(); });
}

void AutonomousWindingWeb::handleList()
{
    if (!m_archive.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_archive_unavailable\"}");
        return;
    }

    uint32_t tolerance = 20UL;
    if (m_server.hasArg("tolerance_percent"))
    {
        if (!parseCanonicalUint32(m_server.arg("tolerance_percent"), tolerance) ||
            tolerance > 50UL)
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_tolerance_percent\"}");
            return;
        }
    }

    const String program = m_server.hasArg("program")
        ? m_server.arg("program") : String();
    String response = F("{\"items\":[");
    response.reserve(8192U);
    uint16_t count = 0U;
    if (!m_archive.appendTasksJson(response,
                                   program,
                                   static_cast<uint8_t>(tolerance),
                                   count))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_program_or_archive_read_failed\"}");
        return;
    }
    response += F("],\"count\":");
    response += count;
    response += F(",\"tolerance_percent\":");
    response += tolerance;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void AutonomousWindingWeb::handleAssign()
{
    if (!m_archive.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_archive_unavailable\"}");
        return;
    }
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }

    uint32_t sessionId = 0UL;
    uint32_t runId = 0UL;
    uint32_t motorId = 0UL;
    if (!m_server.hasArg("session_id") ||
        !m_server.hasArg("run_id") ||
        !m_server.hasArg("motor_id") ||
        !parseCanonicalUint32(m_server.arg("session_id"), sessionId) ||
        !parseCanonicalUint32(m_server.arg("run_id"), runId) ||
        !parseCanonicalUint32(m_server.arg("motor_id"), motorId) ||
        sessionId == 0UL || runId == 0UL || motorId == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_session_run_or_motor_id\"}");
        return;
    }

    const String role = m_server.hasArg("role")
        ? m_server.arg("role") : String();
    if (role != "WORKING" && role != "STARTING" && role != "AUXILIARY")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_assignment_role\"}");
        return;
    }
    if (!m_registry.motorExists(motorId))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }

    bool taskFound = false;
    if (!m_archive.completedTaskExists(sessionId, runId, taskFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_archive_integrity_failed\"}");
        return;
    }
    if (!taskFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_not_found\"}");
        return;
    }

    uint32_t assignmentId = 0UL;
    if (!m_archive.assignMotor(sessionId, runId, motorId, role, assignmentId))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_assignment_failed\"}");
        return;
    }

    String response = F("{\"assigned\":true,\"assignment_id\":");
    response += assignmentId;
    response += F(",\"session_id\":");
    response += sessionId;
    response += F(",\"run_id\":");
    response += runId;
    response += F(",\"motor_id\":");
    response += motorId;
    response += F(",\"role\":\"");
    response += role;
    response += F("\"}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

bool AutonomousWindingWeb::parseCanonicalUint32(const String& text,
                                                uint32_t& value)
{
    value = 0UL;
    if (text.length() == 0U ||
        (text.length() > 1U && text[0] == '0'))
    {
        return false;
    }
    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < text.length(); ++index)
    {
        const char ch = text[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}
}
