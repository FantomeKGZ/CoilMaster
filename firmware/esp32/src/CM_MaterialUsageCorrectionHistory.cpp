#include "CM_MaterialLedger.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_MaterialUsageCorrectionIntegrityAudit.h"

namespace CM
{
namespace
{
bool findUnsigned64Value(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U])) return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

void appendUInt64(String& json, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    json += buffer;
}
}

bool MaterialLedger::appendUsageCorrectionHistoryPageJson(String& json,
                                                          uint32_t repairId,
                                                          uint32_t sourceUsageId,
                                                          uint32_t cursor,
                                                          uint8_t limit,
                                                          uint16_t& count,
                                                          uint32_t& nextCursor,
                                                          bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || repairId == 0UL || limit == 0U || limit > MaxListPageSize)
        return false;
    if (!MaterialUsageCorrectionIntegrityAudit::check(m_storage)) return false;
    if (!m_storage.exists(AdjustmentsPath)) return true;

    File file = m_storage.open(AdjustmentsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    bool first = true;
    uint32_t previousAdjustmentId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t adjustmentId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
            adjustmentId <= previousAdjustmentId)
        {
            file.close();
            return false;
        }
        previousAdjustmentId = adjustmentId;
        if (adjustmentId <= cursor ||
            line.indexOf(F("\"correction_source_usage_id\":")) < 0)
            continue;

        uint32_t lineRepairId = 0UL, lineSourceUsageId = 0UL, materialId = 0UL;
        uint32_t quantity = 0UL, stockBefore = 0UL, stockAfter = 0UL;
        uint64_t lineCost = 0ULL;
        String operationId, timestamp, comment;
        if (!findUnsigned(line, "correction_repair_id", lineRepairId) ||
            !findUnsigned(line, "correction_source_usage_id", lineSourceUsageId) ||
            !findUnsigned(line, "material_id", materialId) ||
            !findUnsigned(line, "correction_quantity_milli", quantity) ||
            !findUnsigned(line, "stock_before_milli", stockBefore) ||
            !findUnsigned(line, "stock_after_milli", stockAfter) ||
            !findUnsigned64Value(line, "correction_line_cost_minor", lineCost) ||
            !findString(line, "correction_operation_id", operationId) ||
            !findString(line, "timestamp", timestamp))
        {
            file.close();
            return false;
        }
        if (lineRepairId != repairId ||
            (sourceUsageId != 0UL && lineSourceUsageId != sourceUsageId))
            continue;

        if (count >= limit)
        {
            hasMore = true;
            break;
        }

        if (!first) json += ',';
        first = false;
        json += F("{\"adjustment_id\":"); json += adjustmentId;
        json += F(",\"source_usage_id\":"); json += lineSourceUsageId;
        json += F(",\"repair_id\":"); json += lineRepairId;
        json += F(",\"material_id\":"); json += materialId;
        json += F(",\"quantity_milli\":"); json += quantity;
        json += F(",\"line_cost_minor\":"); appendUInt64(json, lineCost);
        json += F(",\"stock_before_milli\":"); json += stockBefore;
        json += F(",\"stock_after_milli\":"); json += stockAfter;
        json += F(",\"operation_id\":\""); json += jsonEscape(operationId);
        json += F("\",\"timestamp\":\""); json += jsonEscape(timestamp); json += '"';
        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
            json += F(",\"comment\":\""); json += jsonEscape(comment); json += '"';
        }
        json += '}';
        nextCursor = adjustmentId;
        ++count;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}
}
