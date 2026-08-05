#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::adjustMaterial(const MaterialAdjustment& adjustment,
                                    MaterialAdjustmentResult& result)
{
    result = MaterialAdjustmentResult();
    if (!m_ready || adjustment.materialId == 0UL ||
        (adjustment.addQuantityMilli == 0UL && adjustment.newPricePerUnitMinor == 0UL) ||
        adjustment.currency.length() != 3U || adjustment.timestamp.length() < 10U ||
        !m_storage.exists(MaterialsPath))
    {
        return false;
    }

    uint32_t adjustmentId = 0UL;
    if (!nextId(AdjustmentsPath, "adjustment_id", adjustmentId)) return false;

    File source = m_storage.open(MaterialsPath, FILE_READ);
    if (!source) return false;
    m_storage.remove(MaterialsTempPath);
    File target = m_storage.open(MaterialsTempPath, FILE_WRITE);
    if (!target) { source.close(); return false; }

    bool found = false;
    bool valid = true;
    uint32_t previousStock = 0UL;
    uint32_t previousPrice = 0UL;
    String previousCurrency;

    while (source.available())
    {
        String line = source.readStringUntil('\n');
        uint32_t currentId = 0UL;
        if (findUnsigned(line, "material_id", currentId) &&
            currentId == adjustment.materialId)
        {
            String status;
            if (!findUnsigned(line, "stock_quantity_milli", previousStock) ||
                !findUnsigned(line, "price_per_unit_minor", previousPrice) ||
                !findString(line, "currency", previousCurrency) ||
                (findString(line, "status", status) && status != "ACTIVE"))
            {
                valid = false;
                break;
            }

            const uint64_t newStock = static_cast<uint64_t>(previousStock) +
                                      adjustment.addQuantityMilli;
            if (newStock > 0xFFFFFFFFULL)
            {
                valid = false;
                break;
            }

            result.stockQuantityMilli = static_cast<uint32_t>(newStock);
            result.pricePerUnitMinor = adjustment.newPricePerUnitMinor > 0UL
                                           ? adjustment.newPricePerUnitMinor
                                           : previousPrice;
            result.currency = adjustment.newPricePerUnitMinor > 0UL
                                  ? adjustment.currency
                                  : previousCurrency;

            line = replaceUnsigned(line, "stock_quantity_milli",
                                   result.stockQuantityMilli);
            line = replaceUnsigned(line, "price_per_unit_minor",
                                   result.pricePerUnitMinor);
            line = replaceString(line, "currency", result.currency);
            found = true;
        }

        if (target.println(line) == 0U)
        {
            valid = false;
            break;
        }
    }

    source.close();
    target.flush();
    target.close();
    if (!valid || !found)
    {
        m_storage.remove(MaterialsTempPath);
        return false;
    }

    m_storage.remove(MaterialsPath);
    if (!m_storage.rename(MaterialsTempPath, MaterialsPath)) return false;

    File audit = m_storage.open(AdjustmentsPath, FILE_APPEND);
    if (!audit) return false;

    String line;
    line.reserve(420U);
    line = F("{\"adjustment_id\":"); line += adjustmentId;
    line += F(",\"material_id\":"); line += adjustment.materialId;
    line += F(",\"type\":\"ADJUSTMENT\",\"status\":\"CONFIRMED\"");
    line += F(",\"added_quantity_milli\":"); line += adjustment.addQuantityMilli;
    line += F(",\"stock_before_milli\":"); line += previousStock;
    line += F(",\"stock_after_milli\":"); line += result.stockQuantityMilli;
    line += F(",\"price_before_minor\":"); line += previousPrice;
    line += F(",\"price_after_minor\":"); line += result.pricePerUnitMinor;
    line += F(",\"currency_before\":\""); line += jsonEscape(previousCurrency);
    line += F("\",\"currency_after\":\""); line += jsonEscape(result.currency);
    line += F("\",\"timestamp\":\""); line += jsonEscape(adjustment.timestamp); line += '"';
    if (adjustment.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(adjustment.comment); line += '"';
    }
    line += F("}\n");

    const size_t written = audit.print(line);
    audit.flush();
    audit.close();
    if (written != line.length()) return false;

    result.adjustmentId = adjustmentId;
    return true;
}

String MaterialLedger::replaceUnsigned(const String& line,
                                       const char* key,
                                       uint32_t value)
{
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0) return line;
    int start = pos + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    int end = start;
    while (end < line.length() && isDigit(line[end])) ++end;
    return line.substring(0, start) + String(value) + line.substring(end);
}

String MaterialLedger::replaceString(const String& line,
                                     const char* key,
                                     const String& value)
{
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0) return line;
    const int start = pos + marker.length();
    const int end = line.indexOf('"', start);
    if (end < 0) return line;
    return line.substring(0, start) + jsonEscape(value) + line.substring(end);
}
}
