#include "CM_RepairRegistryWeb.h"
#include <SD.h>
#include "CM_BackupExportWeb.h"
#include "CM_RepairClosureGuard.h"
#include "CM_RepairFinalizationGuard.h"
#include "CM_RepairRegistryLookupWeb.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
bool parseCanonicalUint32Text(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0'))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}

bool validIsoDate(const String& value)
{
    if (value.length() != 10U || value[4] != '-' || value[7] != '-')
        return false;
    for (uint8_t i = 0U; i < 10U; ++i)
    {
        if (i == 4U || i == 7U) continue;
        if (!isDigit(value[i])) return false;
    }
    const uint16_t year = static_cast<uint16_t>(value.substring(0U, 4U).toInt());
    const uint8_t month = static_cast<uint8_t>(value.substring(5U, 7U).toInt());
    const uint8_t day = static_cast<uint8_t>(value.substring(8U, 10U).toInt());
    if (year < 2000U || year > 2199U || month < 1U || month > 12U || day < 1U)
        return false;
    static constexpr uint8_t daysPerMonth[] =
        {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint8_t maximum = daysPerMonth[month - 1U];
    const bool leap = (year % 4U == 0U && year % 100U != 0U) ||
                      year % 400U == 0U;
    if (month == 2U && leap) maximum = 29U;
    return day <= maximum;
}

bool parsePaging(WebServer& server,
                 uint32_t& cursor,
                 uint8_t& limit)
{
    cursor = 0UL;
    limit = 20U;

    if (server.hasArg("cursor") &&
        !parseCanonicalUint32Text(server.arg("cursor"), cursor))
    {
        return false;
    }

    if (server.hasArg("limit"))
    {
        uint32_t parsedLimit = 0UL;
        if (!parseCanonicalUint32Text(server.arg("limit"), parsedLimit) ||
            parsedLimit == 0UL || parsedLimit > RepairRegistry::MaxListPageSize)
        {
            return false;
        }
        limit = static_cast<uint8_t>(parsedLimit);
    }
    return true;
}

void appendPageMetadata(String& response,
                        uint16_t count,
                        uint8_t limit,
                        uint32_t cursor,
                        bool hasMore,
                        uint32_t nextCursor)
{
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
    response += F(",\"max_page_size\":");
    response += RepairRegistry::MaxListPageSize;
    response += '}';
}
}

RepairRegistryWeb::RepairRegistryWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry) {}

void RepairRegistryWeb::begin()
{
    static BackupExportWeb backupExportWeb(m_server, SD);
    static RepairRegistryLookupWeb lookupWeb(m_server, m_registry);
    backupExportWeb.begin();
    lookupWeb.begin();
    m_server.on("/api/clients", HTTP_GET, [this]() { handleListClients(); });
    m_server.on("/api/clients", HTTP_POST, [this]() { handleCreateClient(); });
    m_server.on("/api/motors", HTTP_GET, [this]() { handleListMotors(); });
    m_server.on("/api/motors", HTTP_POST, [this]() { handleCreateMotor(); });
    m_server.on("/api/repairs", HTTP_GET, [this]() { handleListRepairs(); });
    m_server.on("/api/repairs", HTTP_POST, [this]() { handleCreateRepair(); });
    m_server.on("/api/repairs/finalization", HTTP_GET,
                [this]() { handleRepairFinalization(); });
    m_server.on("/api/repairs/close", HTTP_POST,
                [this]() { handleCloseRepair(); });
}

void RepairRegistryWeb::handleListClients()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(limit) * 320U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    const String query = m_server.hasArg("phone") ? m_server.arg("phone") : String();
    if (!m_registry.appendClientsPageJson(response,
                                          query,
                                          cursor,
                                          limit,
                                          count,
                                          nextCursor,
                                          hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"clients_read_failed\"}");
        return;
    }
    appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateClient()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name") || !m_server.hasArg("phone") ||
        m_server.arg("name").length() == 0U ||
        RepairRegistry::normalizePhone(m_server.arg("phone")).length() < 7U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"name_and_valid_phone_required\"}");
        return;
    }
    NewClient client;
    client.name = m_server.arg("name");
    client.phone = m_server.arg("phone");
    client.comment = m_server.arg("comment");
    uint32_t clientId = 0UL;
    if (!m_registry.addClient(client, clientId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"client_write_failed\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"client_id\":");
    response += clientId;
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleListMotors()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String query;
    if (m_server.hasArg("q")) query = m_server.arg("q");
    else if (m_server.hasArg("name")) query = m_server.arg("name");

    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(limit) * 560U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_registry.appendMotorsPageJson(response,
                                         query,
                                         cursor,
                                         limit,
                                         count,
                                         nextCursor,
                                         hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"motors_read_failed\"}");
        return;
    }
    appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateMotor()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name") || m_server.arg("name").length() == 0U ||
        !m_server.hasArg("coil_program") ||
        m_server.arg("coil_program").length() == 0U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"name_and_coil_program_required\"}");
        return;
    }
    if (!WindingProgramParser::valid(m_server.arg("coil_program")))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_coil_program\"}");
        return;
    }
    NewMotor motor;
    motor.name = m_server.arg("name");
    motor.model = m_server.arg("model");
    motor.manufacturer = m_server.arg("manufacturer");
    motor.tags = m_server.arg("tags");
    motor.coilProgram = m_server.arg("coil_program");
    motor.comment = m_server.arg("comment");

    auto optionalUnsigned = [this](const char* name,
                                   uint32_t maximum,
                                   uint32_t& value) -> bool
    {
        value = 0UL;
        if (!m_server.hasArg(name) || m_server.arg(name).length() == 0U)
            return true;
        return parseUnsigned(m_server, name, 1UL, maximum, value);
    };
    uint32_t parsed = 0UL;
    if (!optionalUnsigned("rated_power_w", 100000000UL, motor.ratedPowerW) ||
        !optionalUnsigned("rated_voltage_v", 50000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.ratedVoltageV = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("rated_current_ma", 100000000UL, motor.ratedCurrentMa) ||
        !optionalUnsigned("rated_speed_rpm", 60000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.ratedSpeedRpm = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("frequency_hz", 1000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.frequencyHz = static_cast<uint16_t>(parsed);

    const bool hasPhaseCount = m_server.hasArg("phase_count") &&
                               m_server.arg("phase_count").length() > 0U;
    const bool hasLegacyPhases = m_server.hasArg("phases") &&
                                 m_server.arg("phases").length() > 0U;
    if (hasPhaseCount && hasLegacyPhases &&
        m_server.arg("phase_count") != m_server.arg("phases"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"conflicting_phase_count\"}");
        return;
    }
    parsed = 0UL;
    if ((hasPhaseCount || hasLegacyPhases) &&
        !parseUnsigned(m_server, hasPhaseCount ? "phase_count" : "phases",
                       1UL, 255UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.phases = static_cast<uint8_t>(parsed);

    if (!optionalUnsigned("slot_count", 1000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.slotCount = static_cast<uint16_t>(parsed);

    parsed = 1UL;
    if (m_server.hasArg("repeat_target") &&
        !parseUnsigned(m_server, "repeat_target", 1UL, 0xFFFFUL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repeat_target\"}");
        return;
    }
    motor.repeatTarget = static_cast<uint16_t>(parsed);

    if (!optionalUnsigned("pole_count", 128UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.poleCount = static_cast<uint8_t>(parsed);
    if (!optionalUnsigned("coil_pitch", 1000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.coilPitch = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("turns_per_coil", 9999UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.turnsPerCoil = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("wire_diameter_hundredths_mm", 10000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.wireDiameterHundredthsMm = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("parallel_strands", 255UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.parallelStrands = static_cast<uint8_t>(parsed);
    if (!optionalUnsigned("stator_bore_mm", 10000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.statorBoreMm = static_cast<uint16_t>(parsed);
    if (!optionalUnsigned("stator_core_length_mm", 10000UL, parsed))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_numeric_field\"}");
        return;
    }
    motor.statorCoreLengthMm = static_cast<uint16_t>(parsed);

    motor.connection = m_server.arg("connection");
    motor.connection.trim();
    motor.connection.toUpperCase();
    motor.windingType = m_server.arg("winding_type");
    motor.wireMaterial = m_server.arg("wire_material");
    motor.wireMaterial.trim();
    motor.wireMaterial.toUpperCase();
    motor.sourceType = m_server.arg("source_type");
    motor.sourceType.trim();
    motor.sourceType.toUpperCase();
    motor.sourceUrl = m_server.arg("source_url");
    motor.sourceTitle = m_server.arg("source_title");
    motor.sourceRetrievedAt = m_server.arg("source_retrieved_at");
    motor.confidence = m_server.arg("confidence");
    motor.confidence.trim();
    motor.confidence.toUpperCase();

    motor.name.trim();
    motor.model.trim();
    motor.manufacturer.trim();
    motor.tags.trim();
    motor.comment.trim();
    motor.windingType.trim();
    motor.sourceUrl.trim();
    motor.sourceTitle.trim();
    motor.sourceRetrievedAt.trim();
    if (motor.name.length() == 0U || motor.name.length() > 120U ||
        motor.model.length() > 80U || motor.manufacturer.length() > 80U ||
        motor.tags.length() > 240U || motor.comment.length() > 500U ||
        motor.windingType.length() > 80U || motor.sourceUrl.length() > 300U ||
        motor.sourceTitle.length() > 200U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"motor_text_field_too_long\"}");
        return;
    }

    const bool validConnection = motor.connection.length() == 0U ||
                                 motor.connection == "Y" ||
                                 motor.connection == "DELTA" ||
                                 motor.connection == "Y/DELTA";
    const bool validWireMaterial = motor.wireMaterial.length() == 0U ||
                                   motor.wireMaterial == "CU" ||
                                   motor.wireMaterial == "AL";
    const bool validSourceType = motor.sourceType.length() == 0U ||
                                 motor.sourceType == "MANUFACTURER" ||
                                 motor.sourceType == "TECHNICAL_REFERENCE" ||
                                 motor.sourceType == "REPAIR_RECORD" ||
                                 motor.sourceType == "CALCULATED" ||
                                 motor.sourceType == "UNVERIFIED";
    const bool validConfidence = motor.confidence.length() == 0U ||
                                 motor.confidence == "VERIFIED" ||
                                 motor.confidence == "CORROBORATED" ||
                                 motor.confidence == "CALCULATED" ||
                                 motor.confidence == "UNVERIFIED";
    if (!validConnection || !validWireMaterial ||
        !validSourceType || !validConfidence)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_motor_classification_field\"}");
        return;
    }

    if (m_server.hasArg("calculated_fields"))
    {
        const String value = m_server.arg("calculated_fields");
        if (value != "true" && value != "false" &&
            value != "1" && value != "0")
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_calculated_fields\"}");
            return;
        }
        motor.calculatedFields = value == "true" || value == "1";
    }

    const bool hasSourceMetadata = motor.sourceType.length() > 0U ||
                                   motor.sourceUrl.length() > 0U ||
                                   motor.sourceTitle.length() > 0U ||
                                   motor.sourceRetrievedAt.length() > 0U ||
                                   motor.confidence.length() > 0U ||
                                   motor.calculatedFields;
    if (hasSourceMetadata &&
        (motor.sourceType.length() == 0U ||
         motor.sourceTitle.length() == 0U ||
         motor.confidence.length() == 0U))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"source_type_title_and_confidence_required\"}");
        return;
    }
    if (motor.sourceRetrievedAt.length() > 0U &&
        !validIsoDate(motor.sourceRetrievedAt))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_source_retrieved_at\"}");
        return;
    }
    String sourceUrlScheme = motor.sourceUrl;
    sourceUrlScheme.toLowerCase();
    if (motor.sourceUrl.length() > 0U &&
        !sourceUrlScheme.startsWith("http://") &&
        !sourceUrlScheme.startsWith("https://"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_source_url\"}");
        return;
    }
    const bool classifiedAsCalculated = motor.sourceType == "CALCULATED" ||
                                        motor.confidence == "CALCULATED";
    if (classifiedAsCalculated != motor.calculatedFields)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_calculated_provenance\"}");
        return;
    }

    uint32_t motorId = 0UL;
    if (!m_registry.addMotor(motor, motorId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"motor_write_failed\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"motor_id\":");
    response += motorId;
    response += F(",\"repeat_target\":");
    response += motor.repeatTarget;
    response += F(",\"phase_count\":");
    if (motor.phases > 0U) response += motor.phases;
    else response += F("null");
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleListRepairs()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t clientId = 0UL;
    if (m_server.hasArg("client_id") && m_server.arg("client_id").length() > 0U &&
        !parseUnsigned(m_server, "client_id", 1UL, 0xFFFFFFFFUL, clientId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_client_id\"}");
        return;
    }

    String statusFilter;
    if (m_server.hasArg("status"))
    {
        statusFilter = m_server.arg("status");
        if (statusFilter != "OPEN" && statusFilter != "CLOSED")
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_repair_status\"}");
            return;
        }
    }

    uint32_t cursor = 0UL;
    uint8_t limit = 20U;
    if (!parsePaging(m_server, cursor, limit))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_cursor_or_limit\"}");
        return;
    }

    String response = F("{\"items\":[");
    response.reserve(384U + static_cast<unsigned int>(limit) * 520U);
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    if (!m_registry.appendRepairsPageJson(response,
                                          clientId,
                                          statusFilter,
                                          cursor,
                                          limit,
                                          count,
                                          nextCursor,
                                          hasMore))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repairs_read_failed\"}");
        return;
    }
    appendPageMetadata(response, count, limit, cursor, hasMore, nextCursor);
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCreateRepair()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    uint32_t clientId = 0UL;
    uint32_t motorId = 0UL;
    if (!parseUnsigned(m_server, "client_id", 1UL, 0xFFFFFFFFUL, clientId) ||
        !parseUnsigned(m_server, "motor_id", 1UL, 0xFFFFFFFFUL, motorId) ||
        !m_server.hasArg("received_at") || m_server.arg("received_at").length() < 10U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_fields\"}");
        return;
    }
    NewRepair repair;
    repair.clientId = clientId;
    repair.motorId = motorId;
    repair.receivedAt = m_server.arg("received_at");
    repair.complaint = m_server.arg("complaint");
    repair.comment = m_server.arg("comment");
    uint32_t repairId = 0UL;
    if (!m_registry.addRepair(repair, repairId))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"repair_not_created\"}");
        }
        return;
    }
    String response = F("{\"created\":true,\"repair_id\":");
    response += repairId;
    response += F(",\"client_id\":");
    response += clientId;
    response += F(",\"motor_id\":");
    response += motorId;
    response += '}';
    m_server.send(201, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleRepairFinalization()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_id\"}");
        return;
    }

    bool repairOpen = false;
    if (!m_registry.repairIsOpen(repairId, repairOpen))
    {
        if (!m_registry.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repair_close_state_read_failed\"}");
        return;
    }

    if (!repairOpen)
    {
        String response = F("{\"repair_id\":");
        response += repairId;
        response += F(",\"status\":\"CLOSED\",\"ready_to_close\":false,\"already_closed\":true,\"reason\":\"already_closed\"}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    bool closureAllowed = false;
    if (!RepairClosureGuard::canClose(SD, repairId, closureAllowed))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_closure_state_unavailable\"}");
        return;
    }
    if (!closureAllowed)
    {
        String response = F("{\"repair_id\":");
        response += repairId;
        response += F(",\"status\":\"OPEN\",\"ready_to_close\":false,\"already_closed\":false,\"reason\":\"repair_has_unfinished_winding_job\"}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    const RepairFinalizationCheck finalization =
        RepairFinalizationGuard::check(SD, repairId);
    const char* reason = nullptr;
    switch (finalization)
    {
        case RepairFinalizationCheck::Ready:
            break;
        case RepairFinalizationCheck::CostingStorageUnavailable:
            reason = "repair_finalization_costing_storage_unavailable";
            break;
        case RepairFinalizationCheck::CostingIntegrityFailed:
            reason = "repair_finalization_costing_integrity_failed";
            break;
        case RepairFinalizationCheck::WindingStorageUnavailable:
            reason = "repair_finalization_winding_storage_unavailable";
            break;
        case RepairFinalizationCheck::WindingIntegrityFailed:
            reason = "repair_finalization_winding_integrity_failed";
            break;
        case RepairFinalizationCheck::WireWriteOffRequired:
            reason = "repair_finalization_wire_writeoff_required";
            break;
        case RepairFinalizationCheck::WireWriteOffStorageUnavailable:
            reason = "repair_finalization_wire_writeoff_storage_unavailable";
            break;
        case RepairFinalizationCheck::WireWriteOffIntegrityFailed:
            reason = "repair_finalization_wire_writeoff_integrity_failed";
            break;
        default:
            reason = "repair_finalization_unknown_failure";
            break;
    }

    String response = F("{\"repair_id\":");
    response += repairId;
    response += F(",\"status\":\"OPEN\",\"ready_to_close\":");
    response += finalization == RepairFinalizationCheck::Ready ? F("true") : F("false");
    response += F(",\"already_closed\":false,\"reason\":");
    if (reason == nullptr) response += F("null");
    else
    {
        response += '"';
        response += reason;
        response += '"';
    }
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RepairRegistryWeb::handleCloseRepair()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !m_server.hasArg("closed_at") || m_server.arg("closed_at").length() < 10U ||
        !m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_repair_close_request\"}");
        return;
    }

    bool repairOpen = false;
    if (!m_registry.repairIsOpen(repairId, repairOpen))
    {
        if (!m_registry.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"repair_close_state_read_failed\"}");
        return;
    }
    if (!repairOpen)
    {
        String response = F("{\"closed\":true,\"repair_id\":");
        response += repairId;
        response += F(",\"already_closed\":true,\"write_performed\":false,\"finalization_check_skipped\":true}");
        m_server.send(200, "application/json; charset=utf-8", response);
        return;
    }

    bool closureAllowed = false;
    if (!RepairClosureGuard::canClose(SD, repairId, closureAllowed))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_closure_state_unavailable\"}");
        return;
    }
    if (!closureAllowed)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"repair_has_unfinished_winding_job\",\"write_performed\":false}");
        return;
    }

    const RepairFinalizationCheck finalization =
        RepairFinalizationGuard::check(SD, repairId);
    if (finalization == RepairFinalizationCheck::CostingStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_costing_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WindingStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_winding_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WireWriteOffStorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_storage_unavailable\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WireWriteOffRequired)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_required\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::CostingIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_costing_integrity_failed\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WindingIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_winding_integrity_failed\",\"write_performed\":false}");
        return;
    }
    if (finalization == RepairFinalizationCheck::WireWriteOffIntegrityFailed)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_wire_writeoff_integrity_failed\",\"write_performed\":false}");
        return;
    }
    if (finalization != RepairFinalizationCheck::Ready)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"repair_finalization_unknown_failure\",\"write_performed\":false}");
        return;
    }

    bool alreadyClosed = false;
    if (!m_registry.closeRepair(repairId, m_server.arg("closed_at"), alreadyClosed))
    {
        if (!m_registry.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"repair_registry_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"repair_not_closed\"}");
        }
        return;
    }

    String response = F("{\"closed\":true,\"repair_id\":");
    response += repairId;
    response += F(",\"already_closed\":");
    response += alreadyClosed ? F("true") : F("false");
    response += F(",\"write_performed\":true,\"finalization_integrity_verified\":true}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool RepairRegistryWeb::parseUnsigned(WebServer& server, const char* name,
                                      uint32_t minimum, uint32_t maximum,
                                      uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}
}
