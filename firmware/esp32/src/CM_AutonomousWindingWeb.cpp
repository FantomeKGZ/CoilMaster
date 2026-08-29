#include "CM_AutonomousWindingWeb.h"

namespace CM
{
namespace
{
constexpr uint8_t DefaultTaskPageSize = 20U;
}

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
    m_server.on("/api/autonomous-windings/web-completed", HTTP_GET,
                [this]() { handleCompletedWebJobsList(); });
    m_server.on("/api/autonomous-windings/web-completed/assign", HTTP_POST,
                [this]() { handleCompletedWebJobAssign(); });
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

    uint32_t cursor = 0UL;
    if (m_server.hasArg("cursor") &&
        !parseCanonicalUint32(m_server.arg("cursor"), cursor))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor\"}");
        return;
    }

    uint32_t limitValue = DefaultTaskPageSize;
    if (m_server.hasArg("limit"))
    {
        if (!parseCanonicalUint32(m_server.arg("limit"), limitValue) ||
            limitValue == 0UL ||
            limitValue > AutonomousWindingArchive::MaxTaskPageSize)
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_limit\"}");
            return;
        }
    }
    const uint8_t limit = static_cast<uint8_t>(limitValue);

    const String program = m_server.hasArg("program")
        ? m_server.arg("program") : String();

    String response = F("{\"items\":[");
    response.reserve(768U + static_cast<unsigned int>(limit) * 320U);

    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_archive.appendTasksPageJson(response,
                                       program,
                                       static_cast<uint8_t>(tolerance),
                                       cursor,
                                       limit,
                                       count,
                                       nextCursor,
                                       hasMore))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_program_cursor_or_archive_read_failed\"}");
        return;
    }

    response += F("],\"count\":");
    response += count;
    response += F(",\"limit\":");
    response += limit;
    response += F(",\"cursor\":");
    response += cursor;
    response += F(",\"has_more\":");
    response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor;
    else response += F("null");
    response += F(",\"tolerance_percent\":");
    response += tolerance;
    response += F(",\"max_page_size\":");
    response += AutonomousWindingArchive::MaxTaskPageSize;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void AutonomousWindingWeb::handleCompletedWebJobsList()
{
    if (!m_archive.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"autonomous_winding_archive_unavailable\"}");
        return;
    }

    uint32_t cursor = 0UL;
    if (m_server.hasArg("cursor") &&
        !parseCanonicalUint32(m_server.arg("cursor"), cursor))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor\"}");
        return;
    }

    uint32_t limitValue = DefaultTaskPageSize;
    if (m_server.hasArg("limit") &&
        (!parseCanonicalUint32(m_server.arg("limit"), limitValue) ||
         limitValue == 0UL ||
         limitValue > AutonomousWindingArchive::MaxTaskPageSize))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_limit\"}");
        return;
    }
    const uint8_t limit = static_cast<uint8_t>(limitValue);

    String response = F("{\"source\":\"ESP32_JOB\",\"items\":[");
    response.reserve(768U + static_cast<unsigned int>(limit) * 384U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_archive.appendCompletedWebJobsPageJson(response,
                                                  cursor,
                                                  limit,
                                                  count,
                                                  nextCursor,
                                                  hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"completed_web_job_archive_integrity_failed\"}");
        return;
    }

    response += F("],\"count\":"); response += count;
    response += F(",\"limit\":"); response += limit;
    response += F(",\"cursor\":"); response += cursor;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor;
    else response += F("null");
    response += F(",\"max_page_size\":");
    response += AutonomousWindingArchive::MaxTaskPageSize;
    response += F(",\"run_evidence_copied\":false}");
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
    if (role != "WORKING" && role != "STARTING")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_assignment_role\"}");
        return;
    }

    bool replaceExisting = false;
    if (m_server.hasArg("replace_existing"))
    {
        const String replaceValue = m_server.arg("replace_existing");
        if (replaceValue != "true" && replaceValue != "false")
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_replace_existing\"}");
            return;
        }
        replaceExisting = replaceValue == "true";
    }

    bool motorFound = false;
    if (!m_registry.motorExists(motorId, motorFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_lookup_integrity_failed\"}");
        return;
    }
    if (!motorFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }

    uint32_t assignmentId = 0UL;
    const AutonomousWindingAssignResult result =
        m_archive.assignMotorChecked(sessionId,
                                     runId,
                                     motorId,
                                     role,
                                     replaceExisting,
                                     assignmentId);

    switch (result)
    {
        case AutonomousWindingAssignResult::TaskNotFound:
            m_server.send(404, "application/json; charset=utf-8",
                          "{\"error\":\"autonomous_winding_not_found\"}");
            return;
        case AutonomousWindingAssignResult::RoleOccupied:
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"motor_winding_role_occupied\",\"replace_existing_required\":true}");
            return;
        case AutonomousWindingAssignResult::StartingRequiresWorking:
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"starting_requires_existing_working_winding\"}");
            return;
        case AutonomousWindingAssignResult::ProjectionFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motor_winding_projection_failed\"}");
            return;
        case AutonomousWindingAssignResult::ArchiveIntegrityFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"autonomous_winding_archive_integrity_failed\"}");
            return;
        case AutonomousWindingAssignResult::StorageUnavailable:
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"autonomous_winding_archive_unavailable\"}");
            return;
        case AutonomousWindingAssignResult::Invalid:
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_autonomous_winding_assignment\"}");
            return;
        case AutonomousWindingAssignResult::WriteFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"autonomous_winding_assignment_failed\"}");
            return;
        case AutonomousWindingAssignResult::Assigned:
            break;
    }

    String response = F("{\"assigned\":true,\"assignment_id\":");
    response += assignmentId;
    response += F(",\"session_id\":"); response += sessionId;
    response += F(",\"run_id\":"); response += runId;
    response += F(",\"motor_id\":"); response += motorId;
    response += F(",\"role\":\""); response += role;
    response += F("\",\"replace_existing\":");
    response += replaceExisting ? F("true") : F("false");
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void AutonomousWindingWeb::handleCompletedWebJobAssign()
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
    if (!m_server.hasArg("session_id") || !m_server.hasArg("run_id") ||
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

    const String role = m_server.hasArg("role") ? m_server.arg("role") : String();
    if (role != "WORKING" && role != "STARTING")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_assignment_role\"}");
        return;
    }

    bool replaceExisting = false;
    if (m_server.hasArg("replace_existing"))
    {
        const String replaceValue = m_server.arg("replace_existing");
        if (replaceValue != "true" && replaceValue != "false")
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_replace_existing\"}");
            return;
        }
        replaceExisting = replaceValue == "true";
    }

    bool motorFound = false;
    if (!m_registry.motorExists(motorId, motorFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_lookup_integrity_failed\"}");
        return;
    }
    if (!motorFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }

    uint32_t assignmentId = 0UL;
    const AutonomousWindingAssignResult result =
        m_archive.assignCompletedWebJobMotorChecked(sessionId,
                                                    runId,
                                                    motorId,
                                                    role,
                                                    replaceExisting,
                                                    assignmentId);
    switch (result)
    {
        case AutonomousWindingAssignResult::TaskNotFound:
            m_server.send(404, "application/json; charset=utf-8",
                          "{\"error\":\"completed_web_job_not_found\"}");
            return;
        case AutonomousWindingAssignResult::RoleOccupied:
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"motor_winding_role_occupied\",\"replace_existing_required\":true}");
            return;
        case AutonomousWindingAssignResult::StartingRequiresWorking:
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"starting_requires_existing_working_winding\"}");
            return;
        case AutonomousWindingAssignResult::ProjectionFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motor_winding_projection_failed\"}");
            return;
        case AutonomousWindingAssignResult::ArchiveIntegrityFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"completed_web_job_archive_integrity_failed\"}");
            return;
        case AutonomousWindingAssignResult::StorageUnavailable:
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"completed_web_job_archive_unavailable\"}");
            return;
        case AutonomousWindingAssignResult::Invalid:
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"immutable_or_invalid_web_job_linkage\"}");
            return;
        case AutonomousWindingAssignResult::WriteFailed:
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"completed_web_job_assignment_failed\"}");
            return;
        case AutonomousWindingAssignResult::Assigned:
            break;
    }

    String response = F("{\"assigned\":true,\"source\":\"ESP32_JOB\",\"assignment_id\":");
    response += assignmentId;
    response += F(",\"session_id\":"); response += sessionId;
    response += F(",\"run_id\":"); response += runId;
    response += F(",\"motor_id\":"); response += motorId;
    response += F(",\"role\":\""); response += role;
    response += F("\",\"replace_existing\":");
    response += replaceExisting ? F("true") : F("false");
    response += F(",\"run_evidence_modified\":false}");
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
