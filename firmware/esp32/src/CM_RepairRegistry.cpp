#include "CM_RepairRegistry.h"

#include "CM_BackupBusinessDataIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}
}

RepairRegistry::RepairRegistry(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool RepairRegistry::begin()
{
    m_ready = false;
    if (!ensureDirectories()) return false;

    BackupBusinessDataAuditMetrics metrics;
    if (!BackupBusinessDataIntegrityAudit::checkWorkshopRegistry(m_storage, metrics))
        return false;

    m_ready = true;
    return true;
}

bool RepairRegistry::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open("/data/workshop", FILE_READ);
    if (!directory) return false;
    const bool available = directory.isDirectory();
    directory.close();
    return available;
}

bool RepairRegistry::addClient(const NewClient& client, uint32_t& clientId)
{
    clientId = 0UL;
    const String normalized = normalizePhone(client.phone);
    if (!ready() || client.name.length() == 0U || normalized.length() < 7U ||
        !nextId(ClientsPath, "client_id", clientId))
    {
        return false;
    }

    File file = m_storage.open(ClientsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(320U);
    line = F("{\"client_id\":"); line += clientId;
    line += F(",\"name\":\""); line += jsonEscape(client.name);
    line += F("\",\"phone\":\""); line += jsonEscape(client.phone);
    line += F("\",\"phone_normalized\":\""); line += normalized;
    line += F("\",\"status\":\"ACTIVE\"");
    if (client.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(client.comment);
        line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::addMotor(const NewMotor& motor, uint32_t& motorId)
{
    motorId = 0UL;
    String canonicalProgram;
    if (!ready() || motor.name.length() == 0U || motor.repeatTarget == 0U ||
        !WindingProgramParser::canonicalize(motor.coilProgram, canonicalProgram) ||
        !nextId(MotorsPath, "motor_id", motorId))
    {
        return false;
    }

    File file = m_storage.open(MotorsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(1440U);
    line = F("{\"motor_id\":"); line += motorId;
    line += F(",\"name\":\""); line += jsonEscape(motor.name);
    line += F("\",\"model\":\""); line += jsonEscape(motor.model);
    line += F("\",\"manufacturer\":\""); line += jsonEscape(motor.manufacturer);
    line += F("\",\"tags\":\""); line += jsonEscape(motor.tags);
    line += F("\",\"coil_program\":\""); line += canonicalProgram;
    line += F("\",\"repeat_target\":"); line += motor.repeatTarget;
    line += F(",\"status\":\"ACTIVE\"");
    if (motor.ratedPowerW > 0UL)
    {
        line += F(",\"rated_power_w\":"); line += motor.ratedPowerW;
    }
    if (motor.ratedVoltageV > 0U)
    {
        line += F(",\"rated_voltage_v\":"); line += motor.ratedVoltageV;
    }
    if (motor.ratedCurrentMa > 0UL)
    {
        line += F(",\"rated_current_ma\":"); line += motor.ratedCurrentMa;
    }
    if (motor.ratedSpeedRpm > 0U)
    {
        line += F(",\"rated_speed_rpm\":"); line += motor.ratedSpeedRpm;
    }
    if (motor.frequencyHz > 0U)
    {
        line += F(",\"frequency_hz\":"); line += motor.frequencyHz;
    }
    if (motor.phases > 0U)
    {
        line += F(",\"phases\":"); line += motor.phases;
    }
    if (motor.slotCount > 0U)
    {
        line += F(",\"slot_count\":"); line += motor.slotCount;
    }
    if (motor.poleCount > 0U)
    {
        line += F(",\"pole_count\":"); line += motor.poleCount;
    }
    if (motor.coilPitch > 0U)
    {
        line += F(",\"coil_pitch\":"); line += motor.coilPitch;
    }
    if (motor.turnsPerCoil > 0U)
    {
        line += F(",\"turns_per_coil\":"); line += motor.turnsPerCoil;
    }
    if (motor.wireDiameterHundredthsMm > 0U)
    {
        line += F(",\"wire_diameter_hundredths_mm\":");
        line += motor.wireDiameterHundredthsMm;
    }
    if (motor.parallelStrands > 0U)
    {
        line += F(",\"parallel_strands\":"); line += motor.parallelStrands;
    }
    if (motor.statorBoreMm > 0U)
    {
        line += F(",\"stator_bore_mm\":"); line += motor.statorBoreMm;
    }
    if (motor.statorCoreLengthMm > 0U)
    {
        line += F(",\"stator_core_length_mm\":");
        line += motor.statorCoreLengthMm;
    }
    const char* optionalNames[] = {
        "connection", "winding_type", "wire_material", "source_type",
        "source_url", "source_title", "source_retrieved_at", "confidence"
    };
    const String* optionalValues[] = {
        &motor.connection, &motor.windingType, &motor.wireMaterial,
        &motor.sourceType, &motor.sourceUrl, &motor.sourceTitle,
        &motor.sourceRetrievedAt, &motor.confidence
    };
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        if (optionalValues[i]->length() == 0U) continue;
        line += F(",\""); line += optionalNames[i]; line += F("\":\"");
        line += jsonEscape(*optionalValues[i]); line += '"';
    }
    if (motor.calculatedFields)
        line += F(",\"calculated_fields\":true");
    if (motor.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(motor.comment);
        line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::addRepair(const NewRepair& repair, uint32_t& repairId)
{
    repairId = 0UL;
    if (!ready() || repair.clientId == 0UL || repair.motorId == 0UL ||
        repair.receivedAt.length() < 10U ||
        !clientExists(repair.clientId) || !motorExists(repair.motorId) ||
        !nextId(RepairsPath, "repair_id", repairId))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(420U);
    line = F("{\"repair_id\":"); line += repairId;
    line += F(",\"client_id\":"); line += repair.clientId;
    line += F(",\"motor_id\":"); line += repair.motorId;
    line += F(",\"received_at\":\""); line += jsonEscape(repair.receivedAt);
    line += F("\",\"status\":\"OPEN\"");
    if (repair.complaint.length() > 0U)
    {
        line += F(",\"complaint\":\"");
        line += jsonEscape(repair.complaint);
        line += '"';
    }
    if (repair.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(repair.comment);
        line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::closeRepair(uint32_t repairId,
                                 const String& closedAt,
                                 bool& alreadyClosed)
{
    alreadyClosed = false;
    if (!ready() || repairId == 0UL || closedAt.length() < 10U ||
        !idExists(RepairsPath, "repair_id", repairId))
    {
        return false;
    }

    bool closed = false;
    String existingClosedAt;
    if (!repairClosed(repairId, closed, existingClosedAt)) return false;
    if (closed)
    {
        alreadyClosed = true;
        return true;
    }

    File file = m_storage.open(RepairStatusPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(160U);
    line = F("{\"repair_id\":");
    line += repairId;
    line += F(",\"status\":\"CLOSED\",\"closed_at\":\"");
    line += jsonEscape(closedAt);
    line += F("\"}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::clientExists(uint32_t clientId) const
{
    return idExists(ClientsPath, "client_id", clientId);
}

bool RepairRegistry::motorExists(uint32_t motorId) const
{
    return idExists(MotorsPath, "motor_id", motorId);
}

bool RepairRegistry::idExists(const char* path,
                              const char* key,
                              uint32_t id) const
{
    if (!ready() || id == 0UL || !m_storage.exists(path)) return false;
    File file = m_storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t current = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, key, current) || current == 0UL)
        {
            file.close();
            return false;
        }
        if (current == id && ++matches > 1U)
        {
            file.close();
            return false;
        }
    }
    file.close();
    return matches == 1U;
}

String RepairRegistry::normalizePhone(const String& phone)
{
    String result;
    result.reserve(phone.length());
    for (size_t index = 0U; index < phone.length(); ++index)
    {
        if (isDigit(phone[index])) result += phone[index];
    }
    return result;
}

bool RepairRegistry::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool RepairRegistry::repairClosed(uint32_t repairId,
                                  bool& closed,
                                  String& closedAt) const
{
    closed = false;
    closedAt = String();
    if (!ready() || repairId == 0UL) return false;
    if (!m_storage.exists(RepairStatusPath)) return true;

    File file = m_storage.open(RepairStatusPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidateRepairId = 0UL;
        String status;
        String candidateClosedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId) ||
            candidateRepairId == 0UL ||
            !findString(line, "status", status) || status != "CLOSED" ||
            !findString(line, "closed_at", candidateClosedAt) ||
            candidateClosedAt.length() < 10U)
        {
            file.close();
            return false;
        }
        if (candidateRepairId != repairId) continue;
        if (closed)
        {
            file.close();
            return false;
        }
        closed = true;
        closedAt = candidateClosedAt;
    }
    file.close();
    return true;
}

bool RepairRegistry::nextId(const char* path,
                            const char* key,
                            uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(path)) return true;

    File file = m_storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, key, candidate) || candidate == 0UL ||
            candidate <= previousId)
        {
            file.close();
            return false;
        }
        previousId = candidate;
    }
    file.close();

    if (previousId == 0xFFFFFFFFUL) return false;
    id = previousId + 1UL;
    return true;
}

bool RepairRegistry::findUnsigned(const String& line,
                                  const char* key,
                                  uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() &&
        isDigit(line[cursor + 1]))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}'))
    {
        return false;
    }

    value = parsed;
    return true;
}

bool RepairRegistry::findString(const String& line,
                                const char* key,
                                String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"')
        {
            while (cursor < line.length() && line[cursor] == ' ') ++cursor;
            return cursor < line.length() &&
                   (line[cursor] == ',' || line[cursor] == '}');
        }
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

String RepairRegistry::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t index = 0U; index < value.length(); ++index)
    {
        const char ch = value[index];
        if (ch == '\\') escaped += F("\\\\");
        else if (ch == '"') escaped += F("\\\"");
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
}
