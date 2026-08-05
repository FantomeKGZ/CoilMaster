#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::adjustMaterial(const MaterialAdjustment& adjustment,
                                    MaterialAdjustmentResult& result)
{
    result = MaterialAdjustmentResult();
    if (!m_ready || adjustment.materialId == 0UL ||
        (adjustment.addQuantityMilli == 0UL && adjustment.newPricePerUnitMinor == 0UL) ||
        adjustment.currency.length() != 3U || !m_storage.exists(MaterialsPath))
    {
        return false;
    }

    File source = m_storage.open(MaterialsPath, FILE_READ);
    if (!source) return false;
    m_storage.remove(MaterialsTempPath);
    File target = m_storage.open(MaterialsTempPath, FILE_WRITE);
    if (!target) { source.close(); return false; }

    bool found = false;
    bool valid = true;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        uint32_t currentId = 0UL;
        if (findUnsigned(line, "material_id", currentId) &&
            currentId == adjustment.materialId)
        {
            uint32_t stock = 0UL;
            uint32_t price = 0UL;
            String status;
            String currency;
            if (!findUnsigned(line, "stock_quantity_milli", stock) ||
                !findUnsigned(line, "price_per_unit_minor", price) ||
                !findString(line, "currency", currency) ||
                (findString(line, "status", status) && status != "ACTIVE"))
            {
                valid = false;
                break;
            }

            const uint64_t newStock = static_cast<uint64_t>(stock) +
                                      adjustment.addQuantityMilli;
            if (newStock > 0xFFFFFFFFULL)
            {
                valid = false;
                break;
            }

            result.stockQuantityMilli = static_cast<uint32_t>(newStock);
            result.pricePerUnitMinor = adjustment.newPricePerUnitMinor > 0UL
                                           ? adjustment.newPricePerUnitMinor
                                           : price;
            result.currency = adjustment.newPricePerUnitMinor > 0UL
                                  ? adjustment.currency
                                  : currency;

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
    return m_storage.rename(MaterialsTempPath, MaterialsPath);
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
