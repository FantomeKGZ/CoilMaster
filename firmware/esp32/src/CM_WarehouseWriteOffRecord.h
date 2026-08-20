#ifndef CM_WAREHOUSE_WRITE_OFF_RECORD_H
#define CM_WAREHOUSE_WRITE_OFF_RECORD_H

#include <Arduino.h>
#include "CM_FlatJsonObjectValidator.h"
#include "CM_KgQuantity.h"

namespace CM
{
enum class WarehouseWriteOffMode : uint8_t
{
    LegacySpool = 0,
    KgFirst = 1
};

enum class WarehouseWriteOffStockMode : uint8_t
{
    LegacySpool = 0,
    Spool = 1,
    Unallocated = 2
};

struct WarehouseWriteOffRecord
{
    WarehouseWriteOffMode mode;
    WarehouseWriteOffStockMode stockMode;
    uint32_t movementId;
    String status;
    bool hasSpoolId;
    uint32_t spoolId;
    uint32_t repairId;
    bool hasSourceSessionId;
    uint32_t sourceSessionId;
    bool hasSourceRunId;
    uint32_t sourceRunId;
    uint16_t diameterHundredthsMm;
    bool hasWireType;
    String wireType;
    bool hasWeights;
    uint32_t weightBeforeGrams;
    uint32_t weightAfterGrams;
    uint32_t massGrams;
    String quantityKg;
    uint32_t pricePerKgMinor;
    String currency;
    String timestamp;
    String comment;

    WarehouseWriteOffRecord()
        : mode(WarehouseWriteOffMode::LegacySpool),
          stockMode(WarehouseWriteOffStockMode::LegacySpool),
          movementId(0UL),
          hasSpoolId(false),
          spoolId(0UL),
          repairId(0UL),
          hasSourceSessionId(false),
          sourceSessionId(0UL),
          hasSourceRunId(false),
          sourceRunId(0UL),
          diameterHundredthsMm(0U),
          hasWireType(false),
          hasWeights(false),
          weightBeforeGrams(0UL),
          weightAfterGrams(0UL),
          massGrams(0UL),
          pricePerKgMinor(0UL)
    {
    }
};

class WarehouseWriteOffRecordCodec
{
public:
    static bool parse(const String& line, WarehouseWriteOffRecord& record)
    {
        record = WarehouseWriteOffRecord();
        if (!FlatJsonObjectValidator::valid(line)) return false;

        String type;
        uint32_t diameter = 0UL;
        if (!findUnsigned(line, "movement_id", record.movementId) ||
            record.movementId == 0UL ||
            !findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", record.status) ||
            (record.status != "PENDING" && record.status != "CONFIRMED" &&
             record.status != "ABORTED") ||
            !findUnsigned(line, "repair_id", record.repairId) ||
            record.repairId == 0UL ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter > 0xFFFFUL)
        {
            return false;
        }
        record.diameterHundredthsMm = static_cast<uint16_t>(diameter);

        record.hasSpoolId = hasField(line, "spool_id");
        if (record.hasSpoolId &&
            (!findUnsigned(line, "spool_id", record.spoolId) || record.spoolId == 0UL))
            return false;

        record.hasSourceSessionId = hasField(line, "source_session_id");
        if (record.hasSourceSessionId &&
            (!findUnsigned(line, "source_session_id", record.sourceSessionId) ||
             record.sourceSessionId == 0UL))
            return false;

        record.hasSourceRunId = hasField(line, "source_run_id");
        if (record.hasSourceRunId &&
            (!record.hasSourceSessionId ||
             !findUnsigned(line, "source_run_id", record.sourceRunId) ||
             record.sourceRunId == 0UL))
            return false;

        record.hasWireType = hasField(line, "wire_type");
        if (record.hasWireType &&
            (!findString(line, "wire_type", record.wireType) ||
             (record.wireType != "CU" && record.wireType != "AL")))
            return false;

        const bool hasBefore = hasField(line, "weight_before_g");
        const bool hasAfter = hasField(line, "weight_after_g");
        if (hasBefore != hasAfter) return false;
        record.hasWeights = hasBefore;
        if (record.hasWeights &&
            (!findUnsigned(line, "weight_before_g", record.weightBeforeGrams) ||
             record.weightBeforeGrams == 0UL ||
             !findUnsigned(line, "weight_after_g", record.weightAfterGrams) ||
             record.weightAfterGrams >= record.weightBeforeGrams))
            return false;

        if (!findUnsigned(line, "mass_g", record.massGrams) || record.massGrams == 0UL ||
            !findUnsigned(line, "price_per_kg_minor", record.pricePerKgMinor) ||
            record.pricePerKgMinor == 0UL ||
            !findString(line, "currency", record.currency) || record.currency.length() != 3U ||
            !findString(line, "timestamp", record.timestamp) || record.timestamp.length() < 10U)
            return false;

        if (hasField(line, "comment") && !findString(line, "comment", record.comment))
            return false;

        const bool hasMode = hasField(line, "writeoff_mode");
        if (!hasMode)
            return validateLegacy(record, line);

        String mode;
        if (!findString(line, "writeoff_mode", mode) || mode != "KG_FIRST") return false;
        record.mode = WarehouseWriteOffMode::KgFirst;
        return validateKgFirst(record, line);
    }

private:
    static bool validateLegacy(WarehouseWriteOffRecord& record, const String& line)
    {
        if (hasField(line, "stock_mode") || hasField(line, "quantity_kg") ||
            !record.hasSpoolId || !record.hasWeights ||
            record.massGrams != record.weightBeforeGrams - record.weightAfterGrams)
            return false;

        record.mode = WarehouseWriteOffMode::LegacySpool;
        record.stockMode = WarehouseWriteOffStockMode::LegacySpool;
        if (record.status == "CONFIRMED")
            return record.diameterHundredthsMm != 0U && record.hasWireType;
        return record.diameterHundredthsMm == 0U && !record.hasWireType;
    }

    static bool validateKgFirst(WarehouseWriteOffRecord& record, const String& line)
    {
        if (!record.hasSourceSessionId || !record.hasSourceRunId ||
            record.diameterHundredthsMm == 0U || !record.hasWireType)
            return false;

        String stockMode;
        if (!findString(line, "stock_mode", stockMode)) return false;
        if (!findString(line, "quantity_kg", record.quantityKg)) return false;
        uint32_t parsedGrams = 0UL;
        if (!KgQuantity::parseGrams(record.quantityKg, parsedGrams) ||
            parsedGrams != record.massGrams ||
            KgQuantity::canonicalKg(parsedGrams) != record.quantityKg)
            return false;

        if (stockMode == "SPOOL")
        {
            record.stockMode = WarehouseWriteOffStockMode::Spool;
            return record.hasSpoolId && record.hasWeights &&
                   record.massGrams == record.weightBeforeGrams - record.weightAfterGrams;
        }
        if (stockMode == "UNALLOCATED")
        {
            record.stockMode = WarehouseWriteOffStockMode::Unallocated;
            return !record.hasSpoolId && !record.hasWeights;
        }
        return false;
    }

    static bool hasField(const String& line, const char* key)
    {
        const String marker = String("\"") + key + F("\":");
        return line.indexOf(marker) >= 0;
    }

    static bool findUnsigned(const String& line, const char* key, uint32_t& value)
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

    static bool findString(const String& line, const char* key, String& value)
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
};
}

#endif
