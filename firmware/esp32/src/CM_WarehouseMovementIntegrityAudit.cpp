#include "CM_WarehouseMovementIntegrityAudit.h"
#include <Arduino.h>

namespace CM
{
namespace
{
bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
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
}

bool WarehouseMovementIntegrityAudit::check(fs::FS& storage)
{
    constexpr const char* Path = "/data/warehouse/movements.ndjson";
    if (!storage.exists(Path)) return true;
    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t maximumId = 0UL;
    uint32_t pendingId = 0UL;
    uint32_t pendingSpool = 0UL;
    uint32_t pendingRepair = 0UL;
    uint32_t pendingSourceSession = 0UL;
    uint32_t pendingSourceRun = 0UL;
    bool pendingHasSourceSession = false;
    bool pendingHasSourceRun = false;
    uint32_t pendingBefore = 0UL;
    uint32_t pendingAfter = 0UL;
    uint32_t pendingMass = 0UL;
    uint32_t pendingPrice = 0UL;
    String pendingCurrency, pendingTimestamp, pendingComment;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!line.startsWith("{") || !line.endsWith("}"))
        {
            file.close();
            return false;
        }

        uint32_t movementId = 0UL, spoolId = 0UL, repairId = 0UL;
        uint32_t sourceSessionId = 0UL, sourceRunId = 0UL, diameter = 0UL;
        uint32_t before = 0UL, after = 0UL, mass = 0UL, price = 0UL;
        String type, status, currency, timestamp, comment, wireType;
        const bool hasSourceSession = line.indexOf(F("\"source_session_id\":")) >= 0;
        const bool hasSourceRun = line.indexOf(F("\"source_run_id\":")) >= 0;
        const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;

        if (!findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            !findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) || diameter > 0xFFFFUL ||
            !findUnsigned(line, "weight_before_g", before) || before == 0UL ||
            !findUnsigned(line, "weight_after_g", after) || after >= before ||
            !findUnsigned(line, "mass_g", mass) || mass != before - after ||
            !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            (hasSourceSession &&
             (!findUnsigned(line, "source_session_id", sourceSessionId) || sourceSessionId == 0UL)) ||
            (hasSourceRun &&
             (!hasSourceSession || !findUnsigned(line, "source_run_id", sourceRunId) || sourceRunId == 0UL)) ||
            (hasComment && !findString(line, "comment", comment)) ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            return false;
        }

        if (status == "PENDING")
        {
            if (pendingId != 0UL || movementId <= maximumId || diameter != 0UL || hasWireType)
            {
                file.close();
                return false;
            }
            maximumId = movementId;
            pendingId = movementId;
            pendingSpool = spoolId;
            pendingRepair = repairId;
            pendingHasSourceSession = hasSourceSession;
            pendingSourceSession = sourceSessionId;
            pendingHasSourceRun = hasSourceRun;
            pendingSourceRun = sourceRunId;
            pendingBefore = before;
            pendingAfter = after;
            pendingMass = mass;
            pendingPrice = price;
            pendingCurrency = currency;
            pendingTimestamp = timestamp;
            pendingComment = comment;
            continue;
        }

        if (status != "CONFIRMED" && status != "ABORTED")
        {
            file.close();
            return false;
        }
        if (pendingId == 0UL || movementId != pendingId ||
            spoolId != pendingSpool || repairId != pendingRepair ||
            hasSourceSession != pendingHasSourceSession ||
            sourceSessionId != pendingSourceSession ||
            hasSourceRun != pendingHasSourceRun ||
            sourceRunId != pendingSourceRun ||
            before != pendingBefore || after != pendingAfter || mass != pendingMass ||
            price != pendingPrice || currency != pendingCurrency ||
            timestamp != pendingTimestamp || comment != pendingComment)
        {
            file.close();
            return false;
        }

        if (status == "ABORTED")
        {
            if (diameter != 0UL || hasWireType)
            {
                file.close();
                return false;
            }
        }
        else if (diameter == 0UL)
        {
            file.close();
            return false;
        }
        // Legacy CONFIRMED rows may omit wire_type. When present it has already
        // been validated as CU/AL above. New strict writers always include it.

        pendingId = 0UL;
        pendingSourceSession = 0UL;
        pendingSourceRun = 0UL;
        pendingHasSourceSession = false;
        pendingHasSourceRun = false;
    }
    file.close();
    return pendingId == 0UL;
}
}
