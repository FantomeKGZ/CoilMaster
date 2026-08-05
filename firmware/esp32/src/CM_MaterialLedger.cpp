#include "CM_MaterialLedger.h"

namespace CM
{
MaterialLedger::MaterialLedger(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MaterialLedger::begin()
{
    m_ready = ensureDirectories();
    if (m_ready) m_ready = recoverPendingUsage();
    return m_ready;
}

bool MaterialLedger::ready() const
{
    return m_ready;
}

bool MaterialLedger::addMaterial(const NewMaterial& material,
                                 uint32_t& assignedMaterialId)
{
    assignedMaterialId = 0UL;
    if (!m_ready || material.name.length() == 0U ||
        material.pricePerUnitMinor == 0UL || material.currency.length() != 3U)
    {
        return false;
    }

    if (!nextId(MaterialsPath, "material_id", assignedMaterialId)) return false;

    File file = m_storage.open(MaterialsPath, FILE_APPEND);
    if (!file) return false;

    String line;
    line.reserve(360U);
    line = F("{\"material_id\":"); line += assignedMaterialId;
    line += F(",\"name\":\""); line += jsonEscape(material.name);
    line += F("\",\"unit\":\""); line += unitText(material.unit);
    line += F("\",\"stock_quantity_milli\":"); line += material.stockQuantityMilli;
    line += F(",\"price_per_unit_minor\":"); line += material.pricePerUnitMinor;
    line += F(",\"currency\":\""); line += jsonEscape(material.currency);
    line += F("\",\"status\":\"ACTIVE\"");
    if (material.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(material.comment); line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    return written == line.length();
}

bool MaterialLedger::appendMaterialsJson(String& json, uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(MaterialsPath)) return true;

    File file = m_storage.open(MaterialsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        String status;
        findString(line, "status", status);
        if (status.length() > 0U && status != "ACTIVE") continue;
        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }
    file.close();
    return true;
}

bool MaterialLedger::confirmUsage(const RepairMaterialUsage& usage,
                                  RepairMaterialUsageResult& result)
{
    result = RepairMaterialUsageResult();
    if (!m_ready || usage.repairId == 0UL || usage.materialId == 0UL ||
        usage.quantityMilli == 0UL || usage.timestamp.length() < 10U ||
        m_storage.exists(UsagePendingPath))
    {
        return false;
    }

    uint32_t usageId = 0UL;
    if (!nextId(UsagePath, "usage_id", usageId)) return false;

    uint32_t stockBefore = 0UL;
    uint32_t price = 0UL;
    String currency;
    if (!readStockQuantity(usage.materialId, stockBefore) ||
        stockBefore < usage.quantityMilli)
    {
        return false;
    }
    const uint32_t remaining = stockBefore - usage.quantityMilli;

    File materialFile = m_storage.open(MaterialsPath, FILE_READ);
    if (!materialFile) return false;
    bool materialFound = false;
    while (materialFile.available())
    {
        const String materialLine = materialFile.readStringUntil('\n');
        uint32_t materialId = 0UL;
        String status;
        if (findUnsigned(materialLine, "material_id", materialId) &&
            materialId == usage.materialId &&
            findUnsigned(materialLine, "price_per_unit_minor", price) &&
            findString(materialLine, "currency", currency) &&
            (!findString(materialLine, "status", status) || status == "ACTIVE"))
        {
            materialFound = true;
            break;
        }
    }
    materialFile.close();
    if (!materialFound) return false;

    const uint64_t cost =
        static_cast<uint64_t>(usage.quantityMilli) * price / 1000ULL;
    char costBuffer[24];
    snprintf(costBuffer, sizeof(costBuffer), "%llu",
             static_cast<unsigned long long>(cost));

    String line;
    line.reserve(360U);
    line = F("{\"usage_id\":"); line += usageId;
    line += F(",\"repair_id\":"); line += usage.repairId;
    line += F(",\"material_id\":"); line += usage.materialId;
    line += F(",\"quantity_milli\":"); line += usage.quantityMilli;
    line += F(",\"price_per_unit_minor\":"); line += price;
    line += F(",\"line_cost_minor\":"); line += costBuffer;
    line += F(",\"currency\":\""); line += jsonEscape(currency);
    line += F("\",\"timestamp\":\""); line += jsonEscape(usage.timestamp); line += '"';
    if (usage.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(usage.comment); line += '"';
    }
    line += F("}\n");

    if (!writePendingUsage(usageId, usage.materialId,
                           stockBefore, remaining, line))
    {
        return false;
    }

    uint32_t rewrittenBefore = 0UL;
    uint32_t rewrittenRemaining = 0UL;
    uint32_t rewrittenPrice = 0UL;
    String rewrittenCurrency;
    if (!rewriteQuantity(usage.materialId, usage.quantityMilli,
                         rewrittenBefore, rewrittenRemaining,
                         rewrittenPrice, rewrittenCurrency))
    {
        m_storage.remove(UsagePendingPath);
        return false;
    }

    if (!appendUsageLine(line))
    {
        // Leave the pending transaction in place. begin() will finish it after reboot.
        m_ready = false;
        return false;
    }

    if (!m_storage.remove(UsagePendingPath))
    {
        // The usage is already durable. Recovery will only remove the stale marker.
        m_ready = false;
        return false;
    }

    result.usageId = usageId;
    result.remainingQuantityMilli = rewrittenRemaining;
    result.unitPriceMinor = rewrittenPrice;
    result.lineCostMinor = cost;
    result.currency = rewrittenCurrency;
    return true;
}

bool MaterialLedger::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/materials") &&
        !m_storage.mkdir("/data/materials")) return false;
    return true;
}

bool MaterialLedger::recoverPendingUsage()
{
    if (!m_storage.exists(UsagePendingPath)) return true;

    File pending = m_storage.open(UsagePendingPath, FILE_READ);
    if (!pending) return false;
    const String metadata = pending.readStringUntil('\n');
    String usageLine = pending.readStringUntil('\n');
    pending.close();
    if (usageLine.length() == 0U) return false;
    usageLine += '\n';

    uint32_t usageId = 0UL;
    uint32_t materialId = 0UL;
    uint32_t stockBefore = 0UL;
    uint32_t stockAfter = 0UL;
    if (!findUnsigned(metadata, "usage_id", usageId) ||
        !findUnsigned(metadata, "material_id", materialId) ||
        !findUnsigned(metadata, "stock_before_milli", stockBefore) ||
        !findUnsigned(metadata, "stock_after_milli", stockAfter))
    {
        return false;
    }

    if (usageExists(usageId))
    {
        return m_storage.remove(UsagePendingPath);
    }

    uint32_t currentStock = 0UL;
    if (!readStockQuantity(materialId, currentStock)) return false;

    if (currentStock == stockBefore)
    {
        // Stock was never changed; discard the prepared transaction.
        return m_storage.remove(UsagePendingPath);
    }
    if (currentStock != stockAfter) return false;

    if (!appendUsageLine(usageLine)) return false;
    return m_storage.remove(UsagePendingPath);
}

bool MaterialLedger::writePendingUsage(uint32_t usageId,
                                       uint32_t materialId,
                                       uint32_t stockBefore,
                                       uint32_t stockAfter,
                                       const String& usageLine)
{
    m_storage.remove(UsagePendingPath);
    File file = m_storage.open(UsagePendingPath, FILE_WRITE);
    if (!file) return false;
    String metadata;
    metadata.reserve(160U);
    metadata = F("{\"usage_id\":"); metadata += usageId;
    metadata += F(",\"material_id\":"); metadata += materialId;
    metadata += F(",\"stock_before_milli\":"); metadata += stockBefore;
    metadata += F(",\"stock_after_milli\":"); metadata += stockAfter;
    metadata += F("}\n");
    const size_t expected = metadata.length() + usageLine.length();
    const size_t written = file.print(metadata) + file.print(usageLine);
    file.flush();
    file.close();
    if (written != expected)
    {
        m_storage.remove(UsagePendingPath);
        return false;
    }
    return true;
}

bool MaterialLedger::usageExists(uint32_t usageId) const
{
    if (!m_storage.exists(UsagePath)) return false;
    File file = m_storage.open(UsagePath, FILE_READ);
    if (!file) return false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t existing = 0UL;
        if (findUnsigned(line, "usage_id", existing) && existing == usageId)
        {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool MaterialLedger::readStockQuantity(uint32_t materialId,
                                       uint32_t& quantityMilli) const
{
    quantityMilli = 0UL;
    if (!m_storage.exists(MaterialsPath)) return false;
    File file = m_storage.open(MaterialsPath, FILE_READ);
    if (!file) return false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentId = 0UL;
        if (findUnsigned(line, "material_id", currentId) &&
            currentId == materialId &&
            findUnsigned(line, "stock_quantity_milli", quantityMilli))
        {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool MaterialLedger::appendUsageLine(const String& line)
{
    File file = m_storage.open(UsagePath, FILE_APPEND);
    if (!file) return false;
    const size_t written = file.print(line);
    file.flush();
    file.close();
    return written == line.length();
}

bool MaterialLedger::nextId(const char* path, const char* key, uint32_t& id) const
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
        if (findUnsigned(line, key, candidate) && candidate > maximum)
            maximum = candidate;
    }
    file.close();
    if (maximum == 0xFFFFFFFFUL) return false;
    id = maximum + 1UL;
    return true;
}

bool MaterialLedger::rewriteQuantity(uint32_t materialId,
                                     uint32_t consumeMilli,
                                     uint32_t& stockBeforeMilli,
                                     uint32_t& remainingMilli,
                                     uint32_t& unitPriceMinor,
                                     String& currency)
{
    stockBeforeMilli = 0UL;
    if (!m_storage.exists(MaterialsPath)) return false;
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
        if (findUnsigned(line, "material_id", currentId) && currentId == materialId)
        {
            uint32_t stock = 0UL;
            String status;
            if (!findUnsigned(line, "stock_quantity_milli", stock) ||
                !findUnsigned(line, "price_per_unit_minor", unitPriceMinor) ||
                !findString(line, "currency", currency) ||
                (findString(line, "status", status) && status != "ACTIVE") ||
                stock < consumeMilli)
            {
                valid = false;
                break;
            }
            stockBeforeMilli = stock;
            remainingMilli = stock - consumeMilli;
            const String marker = F("\"stock_quantity_milli\":");
            const int markerPos = line.indexOf(marker);
            int start = markerPos + marker.length();
            int end = start;
            while (end < line.length() && isDigit(line[end])) ++end;
            line = line.substring(0, start) + String(remainingMilli) + line.substring(end);
            found = true;
        }
        if (target.println(line) == 0U) { valid = false; break; }
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

bool MaterialLedger::restoreQuantity(uint32_t materialId, uint32_t quantityMilli)
{
    if (!m_storage.exists(MaterialsPath)) return false;
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
        if (findUnsigned(line, "material_id", currentId) && currentId == materialId)
        {
            uint32_t stock = 0UL;
            if (!findUnsigned(line, "stock_quantity_milli", stock) ||
                stock > 0xFFFFFFFFUL - quantityMilli)
            {
                valid = false;
                break;
            }
            const uint32_t restored = stock + quantityMilli;
            const String marker = F("\"stock_quantity_milli\":");
            const int markerPos = line.indexOf(marker);
            int start = markerPos + marker.length();
            int end = start;
            while (end < line.length() && isDigit(line[end])) ++end;
            line = line.substring(0, start) + String(restored) + line.substring(end);
            found = true;
        }
        if (target.println(line) == 0U) { valid = false; break; }
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

bool MaterialLedger::findUnsigned(const String& line, const char* key,
                                  uint32_t& value)
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

bool MaterialLedger::findString(const String& line, const char* key,
                                String& value)
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

String MaterialLedger::jsonEscape(const String& value)
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

const char* MaterialLedger::unitText(MaterialUnit unit)
{
    switch (unit)
    {
        case MaterialUnit::Gram: return "GRAM";
        case MaterialUnit::Millilitre: return "MILLILITRE";
        case MaterialUnit::Metre: return "METRE";
        case MaterialUnit::SquareMetre: return "SQUARE_METRE";
        case MaterialUnit::Piece:
        default: return "PIECE";
    }
}
}
