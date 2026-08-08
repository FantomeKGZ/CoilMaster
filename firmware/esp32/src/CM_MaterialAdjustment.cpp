#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool MaterialLedger::adjustMaterial(const MaterialAdjustment& adjustment,
                                    MaterialAdjustmentResult& result)
{
    result = MaterialAdjustmentResult();
    if (!ready() || adjustment.materialId == 0UL ||
        (adjustment.addQuantityMilli == 0UL && adjustment.newPricePerUnitMinor == 0UL) ||
        adjustment.currency != "KGS" || adjustment.timestamp.length() < 10U ||
        !m_storage.exists(MaterialsPath) || m_storage.exists(AdjustmentPendingPath))
    {
        return false;
    }

    uint32_t adjustmentId = 0UL;
    if (!nextId(AdjustmentsPath, "adjustment_id", adjustmentId)) return false;

    uint32_t previousStock = 0UL;
    uint32_t previousPrice = 0UL;
    String previousCurrency;
    if (!readMaterialState(adjustment.materialId,
                           previousStock,
                           previousPrice,
                           previousCurrency) ||
        previousCurrency != "KGS")
    {
        return false;
    }

    const uint64_t newStock = static_cast<uint64_t>(previousStock) +
                              adjustment.addQuantityMilli;
    if (newStock > 0xFFFFFFFFULL) return false;

    result.stockQuantityMilli = static_cast<uint32_t>(newStock);
    result.pricePerUnitMinor = adjustment.newPricePerUnitMinor > 0UL
                                   ? adjustment.newPricePerUnitMinor
                                   : previousPrice;
    result.currency = adjustment.newPricePerUnitMinor > 0UL
                          ? adjustment.currency
                          : previousCurrency;

    String auditLine;
    auditLine.reserve(420U);
    auditLine = F("{\"adjustment_id\":"); auditLine += adjustmentId;
    auditLine += F(",\"material_id\":"); auditLine += adjustment.materialId;
    auditLine += F(",\"type\":\"ADJUSTMENT\",\"status\":\"CONFIRMED\"");
    auditLine += F(",\"added_quantity_milli\":"); auditLine += adjustment.addQuantityMilli;
    auditLine += F(",\"stock_before_milli\":"); auditLine += previousStock;
    auditLine += F(",\"stock_after_milli\":"); auditLine += result.stockQuantityMilli;
    auditLine += F(",\"price_before_minor\":"); auditLine += previousPrice;
    auditLine += F(",\"price_after_minor\":"); auditLine += result.pricePerUnitMinor;
    auditLine += F(",\"currency_before\":\""); auditLine += jsonEscape(previousCurrency);
    auditLine += F("\",\"currency_after\":\""); auditLine += jsonEscape(result.currency);
    auditLine += F("\",\"timestamp\":\""); auditLine += jsonEscape(adjustment.timestamp); auditLine += '"';
    if (adjustment.comment.length() > 0U)
    {
        auditLine += F(",\"comment\":\""); auditLine += jsonEscape(adjustment.comment); auditLine += '"';
    }
    auditLine += F("}\n");

    if (!writePendingAdjustment(adjustmentId,
                                adjustment.materialId,
                                previousStock,
                                result.stockQuantityMilli,
                                previousPrice,
                                result.pricePerUnitMinor,
                                previousCurrency,
                                result.currency,
                                auditLine))
    {
        return false;
    }

    File source = m_storage.open(MaterialsPath, FILE_READ);
    if (!source)
    {
        if (ready()) m_storage.remove(AdjustmentPendingPath);
        return false;
    }
    m_storage.remove(MaterialsTempPath);
    File target = m_storage.open(MaterialsTempPath, FILE_WRITE);
    if (!target)
    {
        source.close();
        if (ready()) m_storage.remove(AdjustmentPendingPath);
        return false;
    }

    bool found = false;
    bool valid = true;
    uint32_t previousMaterialId = 0UL;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t currentStock = 0UL;
        uint32_t currentPrice = 0UL;
        String currentCurrency, status;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_id", currentId) || currentId == 0UL ||
            currentId <= previousMaterialId ||
            !findUnsigned(line, "stock_quantity_milli", currentStock) ||
            !findUnsigned(line, "price_per_unit_minor", currentPrice) || currentPrice == 0UL ||
            !findString(line, "currency", currentCurrency) || currentCurrency.length() != 3U ||
            !findString(line, "status", status))
        {
            valid = false;
            break;
        }
        previousMaterialId = currentId;

        if (currentId == adjustment.materialId)
        {
            if (found || status != "ACTIVE" ||
                currentStock != previousStock || currentPrice != previousPrice ||
                currentCurrency != previousCurrency)
            {
                valid = false;
                break;
            }

            line = replaceUnsigned(line, "stock_quantity_milli",
                                   result.stockQuantityMilli);
            line = replaceUnsigned(line, "price_per_unit_minor",
                                   result.pricePerUnitMinor);
            line = replaceString(line, "currency", result.currency);

            uint32_t verifiedStock = 0UL;
            uint32_t verifiedPrice = 0UL;
            String verifiedCurrency;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "stock_quantity_milli", verifiedStock) ||
                verifiedStock != result.stockQuantityMilli ||
                !findUnsigned(line, "price_per_unit_minor", verifiedPrice) ||
                verifiedPrice != result.pricePerUnitMinor ||
                !findString(line, "currency", verifiedCurrency) ||
                verifiedCurrency != result.currency)
            {
                valid = false;
                break;
            }
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
        m_storage.remove(AdjustmentPendingPath);
        return false;
    }

    if (!replaceMaterialsFileFromTemp())
    {
        if (ready()) m_storage.remove(AdjustmentPendingPath);
        return false;
    }

    if (!appendAdjustmentLine(auditLine))
    {
        m_ready = false;
        return false;
    }

    if (!m_storage.remove(AdjustmentPendingPath))
    {
        m_ready = false;
        return false;
    }

    result.adjustmentId = adjustmentId;
    return true;
}

bool MaterialLedger::recoverPendingAdjustment()
{
    if (!m_storage.exists(AdjustmentPendingPath)) return true;

    File pending = m_storage.open(AdjustmentPendingPath, FILE_READ);
    if (!pending) return false;
    const String metadata = pending.readStringUntil('\n');
    String auditLine = pending.readStringUntil('\n');
    pending.close();
    if (!FlatJsonObjectValidator::valid(metadata) ||
        !FlatJsonObjectValidator::valid(auditLine))
    {
        return false;
    }
    auditLine += '\n';

    uint32_t adjustmentId = 0UL;
    uint32_t materialId = 0UL;
    uint32_t stockBefore = 0UL;
    uint32_t stockAfter = 0UL;
    uint32_t priceBefore = 0UL;
    uint32_t priceAfter = 0UL;
    String currencyBefore, currencyAfter;
    if (!findUnsigned(metadata, "adjustment_id", adjustmentId) || adjustmentId == 0UL ||
        !findUnsigned(metadata, "material_id", materialId) || materialId == 0UL ||
        !findUnsigned(metadata, "stock_before_milli", stockBefore) ||
        !findUnsigned(metadata, "stock_after_milli", stockAfter) ||
        !findUnsigned(metadata, "price_before_minor", priceBefore) || priceBefore == 0UL ||
        !findUnsigned(metadata, "price_after_minor", priceAfter) || priceAfter == 0UL ||
        !findString(metadata, "currency_before", currencyBefore) || currencyBefore.length() != 3U ||
        !findString(metadata, "currency_after", currencyAfter) || currencyAfter.length() != 3U)
    {
        return false;
    }

    uint32_t auditAdjustmentId = 0UL;
    uint32_t auditMaterialId = 0UL;
    uint32_t auditStockBefore = 0UL;
    uint32_t auditStockAfter = 0UL;
    uint32_t auditPriceBefore = 0UL;
    uint32_t auditPriceAfter = 0UL;
    String auditCurrencyBefore, auditCurrencyAfter, type, status, timestamp;
    if (!findUnsigned(auditLine, "adjustment_id", auditAdjustmentId) ||
        auditAdjustmentId != adjustmentId ||
        !findUnsigned(auditLine, "material_id", auditMaterialId) ||
        auditMaterialId != materialId ||
        !findString(auditLine, "type", type) || type != "ADJUSTMENT" ||
        !findString(auditLine, "status", status) || status != "CONFIRMED" ||
        !findUnsigned(auditLine, "stock_before_milli", auditStockBefore) ||
        auditStockBefore != stockBefore ||
        !findUnsigned(auditLine, "stock_after_milli", auditStockAfter) ||
        auditStockAfter != stockAfter ||
        !findUnsigned(auditLine, "price_before_minor", auditPriceBefore) ||
        auditPriceBefore != priceBefore ||
        !findUnsigned(auditLine, "price_after_minor", auditPriceAfter) ||
        auditPriceAfter != priceAfter ||
        !findString(auditLine, "currency_before", auditCurrencyBefore) ||
        auditCurrencyBefore != currencyBefore ||
        !findString(auditLine, "currency_after", auditCurrencyAfter) ||
        auditCurrencyAfter != currencyAfter ||
        !findString(auditLine, "timestamp", timestamp) || timestamp.length() < 10U)
    {
        return false;
    }

    bool durable = false;
    if (m_storage.exists(AdjustmentsPath))
    {
        File audit = m_storage.open(AdjustmentsPath, FILE_READ);
        if (!audit) return false;
        uint32_t previousId = 0UL;
        while (audit.available())
        {
            const String line = audit.readStringUntil('\n');
            if (line.length() == 0U) continue;
            uint32_t existing = 0UL;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "adjustment_id", existing) || existing == 0UL ||
                existing <= previousId)
            {
                audit.close();
                return false;
            }
            previousId = existing;
            if (existing == adjustmentId) durable = true;
        }
        audit.close();
    }
    if (durable) return m_storage.remove(AdjustmentPendingPath);

    uint32_t currentStock = 0UL;
    uint32_t currentPrice = 0UL;
    String currentCurrency;
    if (!readMaterialState(materialId, currentStock, currentPrice, currentCurrency))
        return false;

    const bool beforeState = currentStock == stockBefore &&
                             currentPrice == priceBefore &&
                             currentCurrency == currencyBefore;
    const bool afterState = currentStock == stockAfter &&
                            currentPrice == priceAfter &&
                            currentCurrency == currencyAfter;
    if (beforeState) return m_storage.remove(AdjustmentPendingPath);
    if (!afterState) return false;

    if (!appendAdjustmentLine(auditLine)) return false;
    return m_storage.remove(AdjustmentPendingPath);
}

bool MaterialLedger::writePendingAdjustment(uint32_t adjustmentId,
                                            uint32_t materialId,
                                            uint32_t stockBefore,
                                            uint32_t stockAfter,
                                            uint32_t priceBefore,
                                            uint32_t priceAfter,
                                            const String& currencyBefore,
                                            const String& currencyAfter,
                                            const String& auditLine)
{
    if (currencyBefore.length() != 3U || currencyAfter.length() != 3U ||
        auditLine.length() == 0U)
        return false;

    m_storage.remove(AdjustmentPendingPath);
    File file = m_storage.open(AdjustmentPendingPath, FILE_WRITE);
    if (!file) return false;

    String metadata;
    metadata.reserve(280U);
    metadata = F("{\"adjustment_id\":"); metadata += adjustmentId;
    metadata += F(",\"material_id\":"); metadata += materialId;
    metadata += F(",\"stock_before_milli\":"); metadata += stockBefore;
    metadata += F(",\"stock_after_milli\":"); metadata += stockAfter;
    metadata += F(",\"price_before_minor\":"); metadata += priceBefore;
    metadata += F(",\"price_after_minor\":"); metadata += priceAfter;
    metadata += F(",\"currency_before\":\""); metadata += jsonEscape(currencyBefore);
    metadata += F("\",\"currency_after\":\""); metadata += jsonEscape(currencyAfter);
    metadata += F("\"}\n");

    const size_t expected = metadata.length() + auditLine.length();
    const size_t written = file.print(metadata) + file.print(auditLine);
    file.flush();
    file.close();
    if (written != expected)
    {
        m_storage.remove(AdjustmentPendingPath);
        return false;
    }
    return true;
}

bool MaterialLedger::adjustmentExists(uint32_t adjustmentId) const
{
    if (!m_storage.exists(AdjustmentsPath)) return false;
    File file = m_storage.open(AdjustmentsPath, FILE_READ);
    if (!file) return false;
    uint32_t previousId = 0UL;
    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t existing = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "adjustment_id", existing) || existing == 0UL ||
            existing <= previousId)
        {
            file.close();
            return false;
        }
        previousId = existing;
        if (existing == adjustmentId) found = true;
    }
    file.close();
    return found;
}

bool MaterialLedger::readMaterialState(uint32_t materialId,
                                       uint32_t& stockQuantityMilli,
                                       uint32_t& pricePerUnitMinor,
                                       String& currency) const
{
    stockQuantityMilli = 0UL;
    pricePerUnitMinor = 0UL;
    currency = String();
    if (materialId == 0UL || !m_storage.exists(MaterialsPath)) return false;

    File file = m_storage.open(MaterialsPath, FILE_READ);
    if (!file) return false;
    uint32_t previousId = 0UL;
    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentId = 0UL;
        uint32_t stock = 0UL;
        uint32_t price = 0UL;
        String lineCurrency, status;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", lineCurrency) || lineCurrency.length() != 3U ||
            !findString(line, "status", status))
        {
            file.close();
            return false;
        }
        previousId = currentId;
        if (currentId != materialId) continue;
        if (found || status != "ACTIVE")
        {
            file.close();
            return false;
        }
        found = true;
        stockQuantityMilli = stock;
        pricePerUnitMinor = price;
        currency = lineCurrency;
    }
    file.close();
    return found;
}

bool MaterialLedger::appendAdjustmentLine(const String& line)
{
    File file = m_storage.open(AdjustmentsPath, FILE_APPEND);
    if (!file) return false;
    const size_t written = file.print(line);
    file.flush();
    file.close();
    return written == line.length();
}

String MaterialLedger::replaceUnsigned(const String& line,
                                       const char* key,
                                       uint32_t value)
{
    uint32_t current = 0UL;
    if (!findUnsigned(line, key, current)) return line;

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
    String current;
    if (!findString(line, key, current)) return line;

    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0) return line;
    const int start = pos + marker.length();

    int end = start;
    bool escaped = false;
    for (; end < line.length(); ++end)
    {
        const char ch = line[end];
        if (escaped)
        {
            escaped = false;
            continue;
        }
        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"') break;
    }
    if (end >= line.length()) return line;
    return line.substring(0, start) + jsonEscape(value) + line.substring(end);
}
}
