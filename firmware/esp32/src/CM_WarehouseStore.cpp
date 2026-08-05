#include "CM_WarehouseStore.h"

namespace CM
{
WarehouseStore::WarehouseStore(fs::FS& storage)
    : m_storage(storage), m_summary(), m_summaryCount(0U), m_ready(false)
{
}

bool WarehouseStore::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool WarehouseStore::ready() const
{
    return m_ready;
}

bool WarehouseStore::loadSummary(const char* monthPrefix)
{
    if (!m_ready || monthPrefix == nullptr)
    {
        return false;
    }

    clearSummary();
    return readSpools() && readMovements(monthPrefix);
}

bool WarehouseStore::addSpool(const NewWireSpool& spool, uint32_t& assignedSpoolId)
{
    assignedSpoolId = 0UL;
    if (!m_ready || spool.diameterHundredthsMm == 0U || spool.currentWeightGrams == 0UL)
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
    line.reserve(420U);
    line = F("{\"spool_id\":");
    line += assignedSpoolId;
    line += F(",\"diameter_hundredths_mm\":");
    line += spool.diameterHundredthsMm;
    line += F(",\"current_weight_g\":");
    line += spool.currentWeightGrams;
    line += F(",\"status\":\"ACTIVE\"");

    if (spool.wireType.length() > 0U)
    {
        line += F(",\"wire_type\":\""); line += jsonEscape(spool.wireType); line += '"';
    }
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
    if (spool.pricePerKgMinor > 0UL)
    {
        line += F(",\"price_per_kg_minor\":"); line += spool.pricePerKgMinor;
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
        return false;
    }

    return true;
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

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;

        if (!findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            !findUnsigned(line, "current_weight_g", weight))
        {
            continue;
        }

        findString(line, "status", status);
        if (status.length() > 0U && status != "ACTIVE")
        {
            continue;
        }

        WireStockSummary* summary = findOrCreate(static_cast<uint16_t>(diameter));
        if (summary == nullptr) continue;
        summary->remainingGrams += weight;
        if (summary->activeSpoolCount < 255U) ++summary->activeSpoolCount;
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

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t diameter = 0UL;
        uint32_t grams = 0UL;
        String type;
        String timestamp;
        String confirmed;

        if (!findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            !findUnsigned(line, "mass_g", grams) ||
            !findString(line, "type", type))
        {
            continue;
        }

        if (type != "WRITE_OFF")
        {
            continue;
        }

        findString(line, "status", confirmed);
        if (confirmed.length() > 0U && confirmed != "CONFIRMED")
        {
            continue;
        }

        WireStockSummary* summary = findOrCreate(static_cast<uint16_t>(diameter));
        if (summary == nullptr) continue;
        summary->consumedAllTimeGrams += grams;

        if (findString(line, "timestamp", timestamp) && timestamp.startsWith(monthPrefix))
        {
            summary->consumedMonthGrams += grams;
        }
    }

    file.close();
    return true;
}

bool WarehouseStore::nextSpoolId(uint32_t& id) const
{
    id = 1UL;
    if (!m_storage.exists(SpoolsPath))
    {
        return true;
    }

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    uint32_t maximum = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t candidate = 0UL;
        if (findUnsigned(line, "spool_id", candidate) && candidate > maximum)
        {
            maximum = candidate;
        }
    }
    file.close();

    if (maximum == 0xFFFFFFFFUL) return false;
    id = maximum + 1UL;
    return true;
}

bool WarehouseStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0) return false;

    int index = start + marker.length();
    while (index < line.length() && line[index] == ' ') ++index;
    int end = index;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == index) return false;

    value = static_cast<uint32_t>(line.substring(index, end).toInt());
    return true;
}

bool WarehouseStore::findString(const String& line, const char* key, String& value)
{
    const String marker = String("\"") + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0) return false;

    const int valueStart = start + marker.length();
    const int valueEnd = line.indexOf('"', valueStart);
    if (valueEnd < 0) return false;

    value = line.substring(valueStart, valueEnd);
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
