#include "CM_MotorSimilarityWeb.h"

#include <SD.h>

#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
bool parseUnsignedText(const String& text,
                       uint32_t minimum,
                       uint32_t maximum,
                       uint32_t& value)
{
    value = 0UL;
    if (text.length() == 0U ||
        (text.length() > 1U && text[0] == '0'))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < text.length(); ++i)
    {
        const char ch = text[i];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

bool parseOptionalUnsigned(WebServer& server,
                           const char* name,
                           uint32_t maximum,
                           uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name) || server.arg(name).length() == 0U) return true;
    return parseUnsignedText(server.arg(name), 1UL, maximum, value);
}

bool parseMotorUpdateRequest(WebServer& server,
                             uint32_t& motorId,
                             NewMotor& motor,
                             const char*& error)
{
    motorId = 0UL;
    error = nullptr;
    if (!server.hasArg("motor_id") ||
        !parseUnsignedText(server.arg("motor_id"), 1UL, 0xFFFFFFFFUL, motorId))
    {
        error = "invalid_motor_id";
        return false;
    }

    motor.name = server.arg("name");
    motor.model = server.arg("model");
    motor.manufacturer = server.arg("manufacturer");
    motor.tags = server.arg("tags");
    motor.coilProgram = server.arg("coil_program");
    motor.comment = server.arg("comment");
    if (motor.name.length() == 0U || motor.coilProgram.length() == 0U)
    {
        error = "name_and_coil_program_required";
        return false;
    }
    if (!WindingProgramParser::valid(motor.coilProgram))
    {
        error = "invalid_coil_program";
        return false;
    }

    uint32_t parsed = 0UL;
    if (!parseOptionalUnsigned(server, "rated_power_w", 100000000UL,
                               motor.ratedPowerW) ||
        !parseOptionalUnsigned(server, "rated_voltage_v", 50000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.ratedVoltageV = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "rated_current_ma", 100000000UL,
                               motor.ratedCurrentMa) ||
        !parseOptionalUnsigned(server, "rated_speed_rpm", 60000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.ratedSpeedRpm = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "frequency_hz", 1000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.frequencyHz = static_cast<uint16_t>(parsed);

    const bool hasPhaseCount = server.hasArg("phase_count") &&
                               server.arg("phase_count").length() > 0U;
    const bool hasLegacyPhases = server.hasArg("phases") &&
                                 server.arg("phases").length() > 0U;
    if (hasPhaseCount && hasLegacyPhases &&
        server.arg("phase_count") != server.arg("phases"))
    {
        error = "conflicting_phase_count";
        return false;
    }
    parsed = 0UL;
    if ((hasPhaseCount || hasLegacyPhases) &&
        !parseUnsignedText(server.arg(hasPhaseCount ? "phase_count" : "phases"),
                           1UL, 255UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.phases = static_cast<uint8_t>(parsed);

    if (!parseOptionalUnsigned(server, "slot_count", 1000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.slotCount = static_cast<uint16_t>(parsed);

    parsed = 1UL;
    if (server.hasArg("repeat_target") && server.arg("repeat_target").length() > 0U &&
        !parseUnsignedText(server.arg("repeat_target"), 1UL, 0xFFFFUL, parsed))
    {
        error = "invalid_repeat_target";
        return false;
    }
    motor.repeatTarget = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "pole_count", 128UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.poleCount = static_cast<uint8_t>(parsed);

    if (!parseOptionalUnsigned(server, "coil_pitch", 1000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.coilPitch = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "turns_per_coil", 0xFFFFUL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.turnsPerCoil = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "wire_diameter_hundredths_mm", 10000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.wireDiameterHundredthsMm = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "parallel_strands", 255UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.parallelStrands = static_cast<uint8_t>(parsed);

    if (!parseOptionalUnsigned(server, "stator_bore_mm", 10000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.statorBoreMm = static_cast<uint16_t>(parsed);

    if (!parseOptionalUnsigned(server, "stator_core_length_mm", 10000UL, parsed))
    {
        error = "invalid_motor_numeric_field";
        return false;
    }
    motor.statorCoreLengthMm = static_cast<uint16_t>(parsed);

    motor.connection = server.arg("connection");
    motor.windingType = server.arg("winding_type");
    motor.wireMaterial = server.arg("wire_material");
    motor.sourceType = server.arg("source_type");
    motor.sourceUrl = server.arg("source_url");
    motor.sourceTitle = server.arg("source_title");
    motor.sourceRetrievedAt = server.arg("source_retrieved_at");
    motor.confidence = server.arg("confidence");

    const String calculated = server.arg("calculated_fields");
    motor.calculatedFields = calculated == "1" || calculated == "true" ||
                             calculated == "on" || calculated == "yes";
    return true;
}

bool parseConductors(const String& canonical,
                     MotorWindingRoleSpec& role)
{
    role.conductorCount = 0U;
    if (canonical.length() == 0U) return true;

    size_t start = 0U;
    while (start < canonical.length())
    {
        if (role.conductorCount >= MotorWindingRoleSpec::MaxConductors) return false;
        const int plus = canonical.indexOf('+', start);
        const size_t end = plus < 0 ? canonical.length() : static_cast<size_t>(plus);
        if (end <= start) return false;
        const String token = canonical.substring(start, end);
        const int colon = token.indexOf(':');
        const int x = token.indexOf('x', colon + 1);
        if (colon <= 0 || x <= colon + 1 || static_cast<size_t>(x + 1) >= token.length())
            return false;

        WindingConductorSpec& conductor = role.conductors[role.conductorCount];
        conductor.materialClass = token.substring(0U, static_cast<size_t>(colon));
        if (conductor.materialClass != "CU" && conductor.materialClass != "AL") return false;

        uint32_t diameter = 0UL;
        uint32_t strands = 0UL;
        if (!parseUnsignedText(token.substring(static_cast<size_t>(colon + 1),
                                               static_cast<size_t>(x)),
                               1UL, 0xFFFFUL, diameter) ||
            !parseUnsignedText(token.substring(static_cast<size_t>(x + 1)),
                               1UL, 0xFFUL, strands))
        {
            return false;
        }
        conductor.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        conductor.strandCount = static_cast<uint8_t>(strands);
        ++role.conductorCount;
        if (plus < 0) break;
        start = end + 1U;
    }
    return true;
}

bool parseWindingRoleRequest(WebServer& server,
                             const String& roleName,
                             MotorWindingRoleSpec& role,
                             const char*& error)
{
    role = MotorWindingRoleSpec();
    const bool starting = roleName == "STARTING";
    const String presentText = server.hasArg("present") ? server.arg("present") : String("true");
    const bool present = presentText == "true" || presentText == "1" || presentText == "on";
    if (!present && !starting)
    {
        error = "working_role_required";
        return false;
    }
    if (!present)
    {
        role.present = false;
        return true;
    }

    role.present = true;
    role.coilProgram = server.arg("coil_program");
    if (role.coilProgram.length() == 0U || !WindingProgramParser::valid(role.coilProgram))
    {
        error = "invalid_winding_coil_program";
        return false;
    }

    uint32_t parsed = 1UL;
    if (server.hasArg("repeat_target") && server.arg("repeat_target").length() > 0U &&
        !parseUnsignedText(server.arg("repeat_target"), 1UL, 0xFFFFUL, parsed))
    {
        error = "invalid_winding_repeat_target";
        return false;
    }
    role.repeatTarget = static_cast<uint16_t>(parsed);

    parsed = 0UL;
    if (server.hasArg("coil_pitch") && server.arg("coil_pitch").length() > 0U &&
        !parseUnsignedText(server.arg("coil_pitch"), 1UL, 0xFFFFUL, parsed))
    {
        error = "invalid_winding_coil_pitch";
        return false;
    }
    role.coilPitch = static_cast<uint16_t>(parsed);

    if (!parseConductors(server.arg("conductors"), role))
    {
        error = "invalid_winding_conductors";
        return false;
    }
    return true;
}
}

MotorSimilarityWeb::MotorSimilarityWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server),
      m_registry(registry),
      m_windingVersions(SD),
      m_windingVersionsReady(false)
{
}

void MotorSimilarityWeb::begin()
{
    m_windingVersionsReady = m_windingVersions.begin();
    m_server.on("/api/motors/similar", HTTP_GET,
                [this]() { handleLookup(); });
    m_server.on("/api/motors/update", HTTP_POST,
                [this]() { handleUpdate(); });
    m_server.on("/api/motors/winding/role", HTTP_POST,
                [this]() { handleWindingRoleUpdate(); });
}

void MotorSimilarityWeb::handleUpdate()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t motorId = 0UL;
    NewMotor motor;
    const char* error = nullptr;
    if (!parseMotorUpdateRequest(m_server, motorId, motor, error))
    {
        String response = F("{\"error\":\"");
        response += error == nullptr ? "invalid_motor_update" : error;
        response += F("\"}");
        m_server.send(400, "application/json; charset=utf-8", response);
        return;
    }

    bool found = false;
    if (!m_registry.motorExists(motorId, found))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_lookup_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }
    if (!m_registry.updateMotor(motorId, motor))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_update_failed\"}");
        return;
    }

    String response = F("{\"updated\":true,\"motor_id\":");
    response += motorId;
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void MotorSimilarityWeb::handleWindingRoleUpdate()
{
    if (!m_registry.ready() || !m_windingVersionsReady || !m_windingVersions.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_store_unavailable\"}");
        return;
    }

    uint32_t motorId = 0UL;
    uint32_t expectedVersionId = 0UL;
    if (!m_server.hasArg("motor_id") ||
        !parseUnsignedText(m_server.arg("motor_id"), 1UL, 0xFFFFFFFFUL, motorId) ||
        !m_server.hasArg("expected_winding_version_id") ||
        !parseUnsignedText(m_server.arg("expected_winding_version_id"),
                           0UL, 0xFFFFFFFFUL, expectedVersionId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_or_expected_winding_version_id\"}");
        return;
    }

    bool motorFound = false;
    if (!m_registry.motorExists(motorId, motorFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_lookup_failed\"}");
        return;
    }
    if (!motorFound)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"motor_not_found\"}");
        return;
    }

    const String roleName = m_server.arg("role");
    if (roleName != "WORKING" && roleName != "STARTING")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"editable_winding_role_required\"}");
        return;
    }

    NewMotorWindingVersion latest;
    uint32_t latestVersionId = 0UL;
    bool latestFound = false;
    if (!m_windingVersions.loadLatestByMotor(motorId, latest, latestVersionId, latestFound))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_integrity_failed\"}");
        return;
    }

    if ((latestFound && latestVersionId != expectedVersionId) ||
        (!latestFound && expectedVersionId != 0UL))
    {
        String response = F("{\"error\":\"winding_version_conflict\",\"expected_winding_version_id\":");
        response += expectedVersionId;
        response += F(",\"current_winding_version_id\":");
        response += latestFound ? latestVersionId : 0UL;
        response += '}';
        m_server.send(409, "application/json; charset=utf-8", response);
        return;
    }
    if (!latestFound && roleName == "STARTING")
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"working_winding_version_required\",\"current_winding_version_id\":0}");
        return;
    }

    MotorWindingRoleSpec replacement;
    const char* error = nullptr;
    if (!parseWindingRoleRequest(m_server, roleName, replacement, error))
    {
        String response = F("{\"error\":\"");
        response += error == nullptr ? "invalid_winding_role" : error;
        response += F("\"}");
        m_server.send(400, "application/json; charset=utf-8", response);
        return;
    }

    NewMotorWindingVersion next = latestFound ? latest : NewMotorWindingVersion();
    next.motorId = motorId;
    next.previousVersionId = latestFound ? latestVersionId : 0UL;
    next.sourceRepairId = 0UL;
    next.sourceAutonomousSessionId = 0UL;
    next.sourceAutonomousRunId = 0UL;
    next.sourceAutonomousRole = String();
    next.versionKind = "MANUAL_ROLE_EDIT";
    next.createdAt = m_server.arg("created_at");
    next.comment = m_server.arg("version_comment");
    if (next.createdAt.length() < 10U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"created_at_required\"}");
        return;
    }

    if (roleName == "WORKING") next.working = replacement;
    else next.starting = replacement;

    uint32_t newVersionId = 0UL;
    if (!m_windingVersions.append(next, newVersionId))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motor_winding_version_append_failed\"}");
        return;
    }

    String response = F("{\"updated\":true,\"motor_id\":");
    response += motorId;
    response += F(",\"role\":\"");
    response += roleName;
    response += F("\",\"previous_winding_version_id\":");
    response += latestFound ? latestVersionId : 0UL;
    response += F(",\"winding_version_id\":");
    response += newVersionId;
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void MotorSimilarityWeb::handleLookup()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("coil_program"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"coil_program_required\"}");
        return;
    }

    const String coilProgram = m_server.arg("coil_program");
    if (coilProgram.length() == 0U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"coil_program_required\"}");
        return;
    }
    if (!WindingProgramParser::valid(coilProgram))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_coil_program\"}");
        return;
    }

    NewMotor candidate;
    candidate.name = m_server.arg("name");
    candidate.model = m_server.arg("model");
    candidate.manufacturer = m_server.arg("manufacturer");
    candidate.coilProgram = coilProgram;

    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t sameProgramCount = 0U;
    uint16_t identityMatchCount = 0U;
    uint8_t returnedCount = 0U;
    bool itemsTruncated = false;
    if (!m_registry.appendSimilarMotorsJson(response, candidate,
                                            sameProgramCount,
                                            identityMatchCount,
                                            returnedCount,
                                            itemsTruncated))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"similarity_lookup_failed\"}");
        return;
    }
    response += F("],\"same_program_count\":");
    response += sameProgramCount;
    response += F(",\"identity_match_count\":");
    response += identityMatchCount;
    response += F(",\"returned_count\":");
    response += static_cast<unsigned int>(returnedCount);
    response += F(",\"max_items\":");
    response += static_cast<unsigned int>(RepairRegistry::MaxListPageSize);
    response += F(",\"items_truncated\":");
    response += itemsTruncated ? F("true") : F("false");
    response += F(",\"creation_blocked\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}
}
