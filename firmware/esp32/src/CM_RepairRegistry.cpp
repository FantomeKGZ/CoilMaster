#include "CM_RepairRegistry.h"

namespace CM
{
RepairRegistry::RepairRegistry(fs::FS& storage) : m_storage(storage), m_ready(false) {}

bool RepairRegistry::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool RepairRegistry::ready() const { return m_ready; }

bool RepairRegistry::addClient(const NewClient& client, uint32_t& clientId)
{
    clientId = 0UL;
    const String normalized = normalizePhone(client.phone);
    if (!m_ready || client.name.length() == 0U || normalized.length() < 7U ||
        !nextId(ClientsPath, "client_id", clientId)) return false;

    File file = m_storage.open(ClientsPath, FILE_APPEND);
    if (!file) return false;
    String line;
    line.reserve(320U);
    line = F("{\"client_id\":"); line += clientId;
    line += F(",\"name\":\""); line += jsonEscape(client.name);
    line += F("\",\"phone\":\""); line += jsonEscape(client.phone);
    line += F("\",\"phone_normalized\":\""); line += normalized;
    line += F("\",\"status\":\"ACTIVE\"");
    if (client.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(client.comment); line += '"';
    }
    line += F("}\n");
    const size_t written = file.print(line);
    file.flush(); file.close();
    return written == line.length();
}

bool RepairRegistry::addMotor(const NewMotor& motor, uint32_t& motorId)
{
    motorId = 0UL;
    if (!m_ready || motor.name.length() == 0U || motor.coilProgram.length() == 0U ||
        !nextId(MotorsPath, "motor_id", motorId)) return false;

    File file = m_storage.open(MotorsPath, FILE_APPEND);
    if (!file) return false;
    String line;
    line.reserve(360U);
    line = F("{\"motor_id\":"); line += motorId;
    line += F(",\"name\":\""); line += jsonEscape(motor.name);
    line += F("\",\"coil_program\":\""); line += jsonEscape(motor.coilProgram);
    line += F("\",\"status\":\"ACTIVE\"");
    if (motor.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(motor.comment); line += '"';
    }
    line += F("}\n");
    const size_t written = file.print(line);
    file.flush(); file.close();
    return written == line.length();
}

bool RepairRegistry::addRepair(const NewRepair& repair, uint32_t& repairId)
{
    repairId = 0UL;
    if (!m_ready || repair.clientId == 0UL || repair.motorId == 0UL ||
        repair.receivedAt.length() < 10U || !clientExists(repair.clientId) ||
        !motorExists(repair.motorId) ||
        !nextId(RepairsPath, "repair_id", repairId)) return false;

    File file = m_storage.open(RepairsPath, FILE_APPEND);
    if (!file) return false;
    String line;
    line.reserve(420U);
    line = F("{\"repair_id\":"); line += repairId;
    line += F(",\"client_id\":"); line += repair.clientId;
    line += F(",\"motor_id\":"); line += repair.motorId;
    line += F(",\"received_at\":\""); line += jsonEscape(repair.receivedAt);
    line += F("\",\"status\":\"OPEN\"");
    if (repair.complaint.length() > 0U)
    {
        line += F(",\"complaint\":\""); line += jsonEscape(repair.complaint); line += '"';
    }
    if (repair.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(repair.comment); line += '"';
    }
    line += F("}\n");
    const size_t written = file.print(line);
    file.flush(); file.close();
    return written == line.length();
}

bool RepairRegistry::appendClientsJson(String& json, const String& phoneQuery,
                                       uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(ClientsPath)) return true;
    const String query = normalizePhone(phoneQuery);
    File file = m_storage.open(ClientsPath, FILE_READ);
    if (!file) return false;
    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        String normalized;
        if (query.length() > 0U &&
            (!findString(line, "phone_normalized", normalized) ||
             normalized.indexOf(query) < 0)) continue;
        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }
    file.close();
    return true;
}

bool RepairRegistry::appendMotorsJson(String& json, const String& nameQuery,
                                      uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(MotorsPath)) return true;
    String query = nameQuery;
    query.toLowerCase();
    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file) return false;
    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (query.length() > 0U)
        {
            String name;
            if (!findString(line, "name", name)) continue;
            name.toLowerCase();
            if (name.indexOf(query) < 0) continue;
        }
        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }
    file.close();
    return true;
}

bool RepairRegistry::appendRepairsJson(String& json, uint32_t clientId,
                                       uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(RepairsPath)) return true;
    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file) return false;
    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t lineClientId = 0UL;
        if (clientId > 0UL &&
            (!findUnsigned(line, "client_id", lineClientId) || lineClientId != clientId))
            continue;
        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }
    file.close();
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

bool RepairRegistry::idExists(const char* path, const char* key, uint32_t id) const
{
    if (!m_ready || id == 0UL || !m_storage.exists(path)) return false;
    File file = m_storage.open(path, FILE_READ);
    if (!file) return false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t current = 0UL;
        if (findUnsigned(line, key, current) && current == id)
        {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

String RepairRegistry::normalizePhone(const String& phone)
{
    String result;
    result.reserve(phone.length());
    for (size_t i = 0U; i < phone.length(); ++i)
        if (isDigit(phone[i])) result += phone[i];
    return result;
}

bool RepairRegistry::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") && !m_storage.mkdir("/data/workshop")) return false;
    return true;
}

bool RepairRegistry::nextId(const char* path, const char* key, uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(path)) return true;
    File file = m_storage.open(path, FILE_READ);
    if (!file) return false;
    uint32_t maximum = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t candidate = 0UL;
        if (findUnsigned(line, key, candidate) && candidate > maximum) maximum = candidate;
    }
    file.close();
    if (maximum == 0xFFFFFFFFUL) return false;
    id = maximum + 1UL;
    return true;
}

bool RepairRegistry::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0) return false;
    int start = pos + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    int end = start;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == start) return false;
    value = static_cast<uint32_t>(strtoul(line.substring(start, end).c_str(), nullptr, 10));
    return true;
}

bool RepairRegistry::findString(const String& line, const char* key, String& value)
{
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0) return false;
    const int start = pos + marker.length();
    const int end = line.indexOf('"', start);
    if (end < 0) return false;
    value = line.substring(start, end);
    return true;
}

String RepairRegistry::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\') escaped += F("\\\\");
        else if (ch == '"') escaped += F("\\\"");
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
}
