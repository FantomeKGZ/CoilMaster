#include "CM_RepairRegistry.h"

namespace CM
{
RepairRegistry::RepairRegistry(fs::FS& storage) : m_storage(storage), m_ready(false) {}

bool RepairRegistry::begin()
{
    m_ready = false;
    if (!ensureDirectories()) return false;
    if (!validateUniqueIds(ClientsPath, "client_id") ||
        !validateUniqueIds(MotorsPath, "motor_id") ||
        !validateUniqueIds(RepairsPath, "repair_id"))
    {
        return false;
    }
    m_ready = true;
    return true;
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
    line.reserve(560U);
    line = F("{\"motor_id\":"); line += motorId;
    line += F(",\"name\":\""); line += jsonEscape(motor.name);
    line += F("\",\"model\":\""); line += jsonEscape(motor.model);
    line += F("\",\"manufacturer\":\""); line += jsonEscape(motor.manufacturer);
    line += F("\",\"tags\":\""); line += jsonEscape(motor.tags);
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

bool RepairRegistry::appendMotorsJson(String& json, const String& searchQuery,
                                      uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(MotorsPath)) return true;
    String query = searchQuery;
    query.trim();
    query.toLowerCase();
    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file) return false;
    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (query.length() > 0U)
        {
            String searchable;
            String field;
            if (findString(line, "name", field)) searchable += field + ' ';
            if (findString(line, "model", field)) searchable += field + ' ';
            if (findString(line, "manufacturer", field)) searchable += field + ' ';
            if (findString(line, "tags", field)) searchable += field + ' ';
            if (findString(line, "coil_program", field)) searchable += field;
            searchable.toLowerCase();
            if (searchable.indexOf(query) < 0) continue;
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

    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t current = 0UL;
        if (!findUnsigned(line, key, current) || current == 0UL)
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

bool RepairRegistry::validateUniqueIds(const char* path, const char* key) const
{
    if (!m_storage.exists(path)) return true;
    File source = m_storage.open(path, FILE_READ);
    if (!source || source.isDirectory())
    {
        if (source) source.close();
        return false;
    }

    while (source.available())
    {
        const String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (line[0] != '{' || line[line.length() - 1U] != '}')
        {
            source.close();
            return false;
        }

        uint32_t id = 0UL;
        if (!findUnsigned(line, key, id) || id == 0UL)
        {
            source.close();
            return false;
        }

        File duplicateScan = m_storage.open(path, FILE_READ);
        if (!duplicateScan)
        {
            source.close();
            return false;
        }
        uint8_t matches = 0U;
        while (duplicateScan.available())
        {
            const String candidateLine = duplicateScan.readStringUntil('\n');
            if (candidateLine.length() == 0U) continue;
            uint32_t candidate = 0UL;
            if (!findUnsigned(candidateLine, key, candidate) || candidate == 0UL)
            {
                duplicateScan.close();
                source.close();
                return false;
            }
            if (candidate == id && ++matches > 1U)
            {
                duplicateScan.close();
                source.close();
                return false;
            }
        }
        duplicateScan.close();
        if (matches != 1U)
        {
            source.close();
            return false;
        }
    }

    source.close();
    return true;
}

bool RepairRegistry::nextId(const char* path, const char* key, uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(path)) return true;
    if (!validateUniqueIds(path, key)) return false;

    File file = m_storage.open(path, FILE_READ);
    if (!file) return false;
    uint32_t maximum = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!findUnsigned(line, key, candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        if (candidate > maximum) maximum = candidate;
    }
    file.close();
    if (maximum == 0xFFFFFFFFUL) return false;
    id = maximum + 1UL;
    return true;
}

bool RepairRegistry::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() &&
        isDigit(line[cursor + 1])) return false;

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
        (line[cursor] != ',' && line[cursor] != '}')) return false;

    value = parsed;
    return true;
}

bool RepairRegistry::findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0) return false;

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
