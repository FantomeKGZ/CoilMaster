#include "CM_WarehouseStore.h"
#include "CM_WarehouseMovementIntegrityAudit.h"

namespace CM
{
WarehouseStore::WarehouseStore(fs::FS& storage)
    : m_storage(storage), m_summary(), m_summaryCount(0U), m_ready(false)
{
}

bool WarehouseStore::begin()
{
    m_ready = ensureDirectories();
    if (m_ready) m_ready = recoverSpoolFileSwap();
    if (m_ready) m_ready = recoverPendingWriteOff();
    return m_ready;
}

bool WarehouseStore::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open("/data/warehouse", FILE_READ);
    const bool available = directory && directory.isDirectory();
    if (directory) directory.close();
    return available;
}

bool WarehouseStore::loadSummary(const char* monthPrefix)
{
    if (!ready() || monthPrefix == nullptr)
    {
        return false;
    }
    if (!WarehouseMovementIntegrityAudit::check(m_storage))
    {
        return false;
    }

    clearSummary();
    if (!readSpools() || !readMovements(monthPrefix)) return false;

    uint64_t totalRemaining = 0ULL;
    uint64_t totalConsumedMonth = 0ULL;
    uint64_t totalConsumedAllTime = 0ULL;
    for (uint8_t i = 0U; i < m_summaryCount; ++i)
    {
        totalRemaining += static_cast<uint64_t>(m_summary[i].remainingGrams);
        totalConsumedMonth += static_cast<uint64_t>(m_summary[i].consumedMonthGrams);
        totalConsumedAllTime += static_cast<uint64_t>(m_summary[i].consumedAllTimeGrams);
        if (totalRemaining > 0xFFFFFFFFULL ||
            totalConsumedMonth > 0xFFFFFFFFULL ||
            totalConsumedAllTime > 0xFFFFFFFFULL)
        {
            clearSummary();
            return false;
        }
    }
    return true;
}

bool WarehouseStore::addSpool(const NewWireSpool& spool, uint32_t& assignedSpoolId)
{
    assignedSpoolId = 0UL;
    if (!ready() || spool.diameterHundredthsMm == 0U ||
        spool.currentWeightGrams == 0UL ||
        (spool.wireType != "CU" && spool.wireType != "AL"))
    {
        return false;
    }

    if (!nextSpoolId(assignedSpoolId))
    {
        return false;
    }

    File file = m_storage.open(SpoolsPath, FILE_APPEND);
    if (!file)
    {
        assignedSpoolId = 0UL;
        return false;
    }

    String line;
    line.reserve(380U);
    line = F("{\"spool_id\":");
    line += assignedSpoolId;
    line += F(",\"diameter_hundredths_mm\":");
    line += spool.diameterHundredthsMm;
    line += F(",\"current_weight_g\":");
    line += spool.currentWeightGrams;
    line += F(",\"wire_type\":\"");
    line += spool.wireType;
    line += F("\",\"status\":\"ACTIVE\"");

    if (spool.manufacturer.length() > 0U)
    {
        line += F(",\"manufacturer\":\""); line += jsonEscape(spool.manufacturer); line += '"';
    }
    if (spool.supplier.length() > 0U)
    {
        line += F(",\"supplier\":\""); line += jsonEscape(spool.supplier); line += '"';
    }
    if (spool.batch.length() > 0U)
    {
        line += F(",\"batch\":\""); line += jsonEscape(spool.batch); line += '"';
    }
    if (spool.storageLocation.length() > 0U)
    {
        line += F(",\"storage_location\":\""); line += jsonEscape(spool.storageLocation); line += '"';
    }
    if (spool.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(spool.comment); line += '"';
    }

    line += F("}\n");
    const size_t written = file.print(line);
    file.flush();
    file.close();

    if (written != line.length())
    {
        assignedSpoolId = 0UL;
        m_ready = false;
        return false;
    }

    return true;
}

bool WarehouseStore::setWarehousePrice(const WarehousePrice& price)
{
    if (!ready() || price.pricePerKgMinor == 0UL || price.currency.length() != 3U)
    {
        return false;
    }

    WarehousePrice current;
    bool configured = false;
    if (!loadWarehousePrice(current, configured)) return false;
    if (configured &&
        current.pricePerKgMinor == price.pricePerKgMinor &&
        current.currency == price.currency)
    {
        return true;
    }

    File file = m_storage.open(PricePath, FILE_APPEND);
    if (!file) return false;

    String line;
    line.reserve(96U);
    line = F("{\"price_per_kg_minor\":");
    line += price.pricePerKgMinor;
    line += F(",\"currency\":\"");
    line += jsonEscape(price.currency);
    line += F("\"}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool WarehouseStore::loadWarehousePrice(WarehousePrice& price) const
{
    bool configured = false;
    return loadWarehousePrice(price, configured) && configured;
}

uint8_t WarehouseStore::summaryCount() const
{
    return m_summaryCount;
}

bool WarehouseStore::summaryAt(uint8_t index, WireStockSummary& summary) const
{
    if (index >= m_summaryCount)
    {
        return false;
    }

    summary = m_summary[index];
    return true;
}

uint32_t WarehouseStore::totalRemainingGrams() const
{
    uint32_t total = 0UL;
    for (uint8_t i = 0U; i < m_summaryCount; ++i)
    {
        total += m_summary[i].remainingGrams;
    }
    return total;
}

uint32_t WarehouseStore::totalConsumedMonthGrams() const
{
    uint32_t total = 0UL;
    for (uint8_t i = 0U; i < m_summaryCount; ++i)
    {
        total += m_summary[i].consumedMonthGrams;
    }
    return total;
}

uint32_t WarehouseStore::totalConsumedAllTimeGrams() const
{
    uint32_t total = 0UL;
    for (uint8_t i = 0U; i < m_summaryCount; ++i)
    {
        total += m_summary[i].consumedAllTimeGrams;
    }
    return total;
}

bool WarehouseStore::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/warehouse") && !m_storage.mkdir("/data/warehouse")) return false;
    return true;
}

void WarehouseStore::clearSummary()
{
    m_summaryCount = 0U;
    for (uint8_t i = 0U; i < WarehouseMaxDiameters; ++i)
    {
        m_summary[i] = WireStockSummary();
    }
}

WireStockSummary* WarehouseStore::findOrCreate(uint16_t diameterHundredthsMm)
{
    for (uint8_t i = 0U; i < m_summaryCount; ++i)
    {
        if (m_summary[i].diameterHundredthsMm == diameterHundredthsMm)
        {
            return &m_summary[i];
        }
    }

    if (m_summaryCount >= WarehouseMaxDiameters)
    {
        return nullptr;
    }

    WireStockSummary& item = m_summary[m_summaryCount++];
    item.diameterHundredthsMm = diameterHundredthsMm;
    return &item;
}

bool WarehouseStore::readSpools()
{
    if (!m_storage.exists(SpoolsPath))
    {
        File empty = m_storage.open(SpoolsPath, FILE_WRITE);
        if (!empty) return false;
        empty.close();
        return true;
    }

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousSpoolId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;
        String wireType;
        String optional;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousSpoolId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) || status.length() == 0U ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            return false;
        }
        previousSpoolId = spoolId;

        const char* optionalKeys[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t keyIndex = 0U;
             keyIndex < sizeof(optionalKeys) / sizeof(optionalKeys[0]);
             ++keyIndex)
        {
            const String marker = String("\"") + optionalKeys[keyIndex] + F("\":");
            if (line.indexOf(marker) >= 0 &&
                !findString(line, optionalKeys[keyIndex], optional))
            {
                file.close();
                return false;
            }
        }

        if (status != "ACTIVE") continue;

        WireStockSummary* summary = findOrCreate(static_cast<uint16_t>(diameter));
        if (summary == nullptr)
        {
            file.close();
            return false;
        }
        if (summary->remainingGrams > 0xFFFFFFFFUL - weight)
        {
            file.close();
            return false;
        }
        summary->remainingGrams += weight;
        if (summary->activeSpoolCount == 255U)
        {
            file.close();
            return false;
        }
        ++summary->activeSpoolCount;
    }

    file.close();
    return true;
}

bool WarehouseStore::readMovements(const char* monthPrefix)
{
    if (!m_storage.exists(MovementsPath))
    {
        File empty = m_storage.open(MovementsPath, FILE_WRITE);
        if (!empty) return false;
        empty.close();
        return true;
    }

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximumMovementId = 0UL;
    uint32_t pendingId = 0UL;
    uint32_t pendingSpoolId = 0UL;
    uint32_t pendingRepairId = 0UL;
    uint32_t pendingBefore = 0UL;
    uint32_t pendingAfter = 0UL;
    uint32_t pendingMass = 0UL;
    uint32_t pendingPrice = 0UL;
    String pendingCurrency;
    String pendingTimestamp;
    String pendingComment;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t movementId = 0UL;
        uint32_t spoolId = 0UL;
        uint32_t repairId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t before = 0UL;
        uint32_t after = 0UL;
        uint32_t grams = 0UL;
        uint32_t price = 0UL;
        String type, status, currency, timestamp, comment, wireType;
        const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;

        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            !findString(line, "type", type) || type != "WRITE_OFF" ||
            !findString(line, "status", status) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) || diameter > 0xFFFFUL ||
            !findUnsigned(line, "weight_before_g", before) || before == 0UL ||
            !findUnsigned(line, "weight_after_g", after) || after >= before ||
            !findUnsigned(line, "mass_g", grams) || grams != before - after ||
            !findUnsigned(line, "price_per_kg_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            (hasComment && !findString(line, "comment", comment)) ||
            (hasWireType && !findString(line, "wire_type", wireType)))
        {
            file.close();
            return false;
        }

        if (status == "PENDING")
        {
            if (pendingId != 0UL || movementId <= maximumMovementId ||
                diameter != 0UL || hasWireType)
            {
                file.close();
                return false;
            }
            maximumMovementId = movementId;
            pendingId = movementId;
            pendingSpoolId = spoolId;
            pendingRepairId = repairId;
            pendingBefore = before;
            pendingAfter = after;
            pendingMass = grams;
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
            spoolId != pendingSpoolId || repairId != pendingRepairId ||
            before != pendingBefore || after != pendingAfter || grams != pendingMass ||
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
        else
        {
            if (diameter == 0UL ||
                (hasWireType && wireType != "CU" && wireType != "AL"))
            {
                file.close();
                return false;
            }

            WireStockSummary* summary = findOrCreate(static_cast<uint16_t>(diameter));
            if (summary == nullptr ||
                summary->consumedAllTimeGrams > 0xFFFFFFFFUL - grams)
            {
                file.close();
                return false;
            }
            summary->consumedAllTimeGrams += grams;

            if (timestamp.startsWith(monthPrefix))
            {
                if (summary->consumedMonthGrams > 0xFFFFFFFFUL - grams)
                {
                    file.close();
                    return false;
                }
                summary->consumedMonthGrams += grams;
            }
        }

        pendingId = 0UL;
        pendingSpoolId = 0UL;
        pendingRepairId = 0UL;
        pendingBefore = 0UL;
        pendingAfter = 0UL;
        pendingMass = 0UL;
        pendingPrice = 0UL;
        pendingCurrency = String();
        pendingTimestamp = String();
        pendingComment = String();
    }

    file.close();
    return pendingId == 0UL;
}

bool WarehouseStore::nextSpoolId(uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(SpoolsPath)) return true;

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidate = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;
        String wireType;
        String optional;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "spool_id", candidate) || candidate == 0UL ||
            candidate <= previousId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            return false;
        }

        const char* optionalKeys[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t keyIndex = 0U;
             keyIndex < sizeof(optionalKeys) / sizeof(optionalKeys[0]);
             ++keyIndex)
        {
            const String marker = String("\"") + optionalKeys[keyIndex] + F("\":");
            if (line.indexOf(marker) >= 0 &&
                !findString(line, optionalKeys[keyIndex], optional))
            {
                file.close();
                return false;
            }
        }

        previousId = candidate;
    }
    file.close();

    if (previousId == 0xFFFFFFFFUL) return false;
    id = previousId + 1UL;
    return true;
}

bool WarehouseStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;

    int index = start + marker.length();
    while (index < line.length() && line[index] == ' ') ++index;
    if (index >= line.length() || !isDigit(line[index])) return false;
    if (line[index] == '0' && index + 1 < line.length() && isDigit(line[index + 1]))
        return false;

    uint32_t parsed = 0UL;
    int end = index;
    while (end < line.length() && isDigit(line[end]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[end] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++end;
    }
    while (end < line.length() && line[end] == ' ') ++end;
    if (end >= line.length() || (line[end] != ',' && line[end] != '}')) return false;

    value = parsed;
    return true;
}

bool WarehouseStore::findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;

    int index = start + marker.length();
    String parsed;
    parsed.reserve(32U);
    bool closed = false;
    for (; index < line.length(); ++index)
    {
        const char ch = line[index];
        if (ch == '"')
        {
            closed = true;
            ++index;
            break;
        }
        if (ch == '\\')
        {
            ++index;
            if (index >= line.length()) return false;
            const char escaped = line[index];
            if (escaped == '"' || escaped == '\\') parsed += escaped;
            else if (escaped == 'n') parsed += '\n';
            else if (escaped == 'r') parsed += '\r';
            else if (escaped == 't') parsed += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        parsed += ch;
    }
    if (!closed) return false;
    while (index < line.length() && line[index] == ' ') ++index;
    if (index >= line.length() || (line[index] != ',' && line[index] != '}')) return false;

    value = parsed;
    return true;
}

String WarehouseStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        switch (ch)
        {
            case '\\': escaped += F("\\\\"); break;
            case '"': escaped += F("\\\""); break;
            case '\n': escaped += F("\\n"); break;
            case '\r': escaped += F("\\r"); break;
            case '\t': escaped += F("\\t"); break;
            default:
                if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
                break;
        }
    }
    return escaped;
}
}
