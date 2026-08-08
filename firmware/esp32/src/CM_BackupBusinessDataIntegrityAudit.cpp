#include "CM_BackupBusinessDataIntegrityAudit.h"
#include <Arduino.h>
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
constexpr const char* RepairStatusPath = "/data/workshop/repair-status.ndjson";
constexpr const char* PricingPath = "/data/repairs/pricing.ndjson";

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint64_t parsed = 0ULL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide);
    return true;
}

bool findString(const String& line, const char* key, String& value)
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
            return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool validJsonLine(const String& line)
{
    return line.length() >= 2U && line.startsWith("{") && line.endsWith("}");
}

bool idOccursExactlyOnce(fs::FS& storage, const char* path, const char* key, uint32_t id)
{
    if (id == 0UL || !storage.exists(path)) return false;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint8_t matches = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!validJsonLine(line) || !findUnsigned(line, key, candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        if (candidate == id && ++matches > 1U)
        {
            file.close();
            return false;
        }
    }
    file.close();
    return matches == 1U;
}

bool validateUniqueIds(fs::FS& storage, const char* path, const char* key)
{
    if (!storage.exists(path)) return true;
    File source = storage.open(path, FILE_READ);
    if (!source || source.isDirectory())
    {
        if (source) source.close();
        return false;
    }
    while (source.available())
    {
        const String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL;
        if (!validJsonLine(line) || !findUnsigned(line, key, id) || id == 0UL ||
            !idOccursExactlyOnce(storage, path, key, id))
        {
            source.close();
            return false;
        }
    }
    source.close();
    return true;
}

bool validateMotors(fs::FS& storage)
{
    if (!storage.exists(MotorsPath)) return true;
    File file = storage.open(MotorsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        String program;
        if (!validJsonLine(line) || !findString(line, "coil_program", program) ||
            !WindingProgramParser::valid(program))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

bool validateRepairs(fs::FS& storage)
{
    if (!storage.exists(RepairsPath)) return true;
    File file = storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t repairId = 0UL, clientId = 0UL, motorId = 0UL;
        if (!validJsonLine(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !idOccursExactlyOnce(storage, ClientsPath, "client_id", clientId) ||
            !idOccursExactlyOnce(storage, MotorsPath, "motor_id", motorId))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

bool validateRepairStatus(fs::FS& storage)
{
    if (!storage.exists(RepairStatusPath)) return true;
    File file = storage.open(RepairStatusPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t repairId = 0UL;
        String status, closedAt;
        if (!validJsonLine(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findString(line, "status", status) || status != "CLOSED" ||
            !findString(line, "closed_at", closedAt) || closedAt.length() < 10U ||
            !idOccursExactlyOnce(storage, RepairsPath, "repair_id", repairId) ||
            !idOccursExactlyOnce(storage, RepairStatusPath, "repair_id", repairId))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

bool validatePricing(fs::FS& storage)
{
    if (!storage.exists(PricingPath)) return true;
    File file = storage.open(PricingPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t repairId = 0UL;
        uint64_t labour = 0ULL, client = 0ULL;
        String currency, timestamp;
        if (!validJsonLine(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", client) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            !idOccursExactlyOnce(storage, RepairsPath, "repair_id", repairId))
        {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}
}

bool BackupBusinessDataIntegrityAudit::check(fs::FS& storage)
{
    if (!validateUniqueIds(storage, ClientsPath, "client_id") ||
        !validateUniqueIds(storage, MotorsPath, "motor_id") ||
        !validateUniqueIds(storage, RepairsPath, "repair_id") ||
        !validateMotors(storage) ||
        !validateRepairs(storage) ||
        !validateRepairStatus(storage) ||
        !validatePricing(storage))
    {
        return false;
    }
    return true;
}
}
