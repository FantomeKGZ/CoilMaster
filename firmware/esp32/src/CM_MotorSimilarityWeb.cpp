#include "CM_MotorSimilarityWeb.h"
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
}

MotorSimilarityWeb::MotorSimilarityWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry) {}

void MotorSimilarityWeb::begin()
{
    m_server.on("/api/motors/similar", HTTP_GET,
                [this]() { handleLookup(); });
    m_server.on("/api/motors/update", HTTP_POST,
                [this]() { handleUpdate(); });
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
