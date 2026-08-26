#include "CM_RepairDeliveryIntegrityAudit.h"

#include <Arduino.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_RepairLifecycle.h"

namespace CM
{
namespace
{
constexpr const char* DeliveryPath = "/data/workshop/repair-deliveries.ndjson";
constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";

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

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U])) return false;
    uint32_t parsed = 0UL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (escaped)
        {
            if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else if (ch == '\\' || ch == '"') value += ch;
            else return false;
            escaped = false;
            continue;
        }
        if (ch == '\\') { escaped = true; continue; }
        if (ch == '"') return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool repairIdentityMatches(fs::FS& storage,
                           uint32_t repairId,
                           uint32_t clientId,
                           uint32_t motorId)
{
    if (!storage.exists(RepairsPath)) return false;
    File file = storage.open(RepairsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }
    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidateRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId) || candidateRepairId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateRepairId != repairId) continue;
        uint32_t candidateClientId = 0UL, candidateMotorId = 0UL;
        if (found ||
            !findUnsigned(line, "client_id", candidateClientId) ||
            !findUnsigned(line, "motor_id", candidateMotorId) ||
            candidateClientId != clientId || candidateMotorId != motorId)
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();
    return found;
}

bool deliveryUniqueForRepair(fs::FS& storage, uint32_t repairId)
{
    File file = storage.open(DeliveryPath, FILE_READ);
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
        uint32_t candidateRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId) || candidateRepairId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateRepairId == repairId && ++matches > 1U)
        {
            file.close();
            return false;
        }
    }
    file.close();
    return matches == 1U;
}
}

bool RepairDeliveryIntegrityAudit::check(fs::FS& storage)
{
    uint32_t recordCount = 0UL;
    return check(storage, recordCount);
}

bool RepairDeliveryIntegrityAudit::check(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(DeliveryPath)) return true;

    File file = storage.open(DeliveryPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousDeliveryId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t deliveryId = 0UL, repairId = 0UL, clientId = 0UL, motorId = 0UL;
        String deliveredAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "delivery_id", deliveryId) || deliveryId == 0UL ||
            deliveryId <= previousDeliveryId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !findString(line, "delivered_at", deliveredAt) ||
            deliveredAt.length() < 10U || deliveredAt.length() > 32U ||
            !repairIdentityMatches(storage, repairId, clientId, motorId) ||
            !deliveryUniqueForRepair(storage, repairId))
        {
            file.close();
            return false;
        }

        bool repairOpen = false;
        if (!RepairLifecycle::isOpen(storage, repairId, repairOpen) || repairOpen)
        {
            file.close();
            return false;
        }

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment) || comment.length() > 500U)
            {
                file.close();
                return false;
            }
        }

        if (recordCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++recordCount;
        previousDeliveryId = deliveryId;
    }

    file.close();
    return true;
}
}
