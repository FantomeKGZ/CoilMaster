#include "CM_RepairCosting.h"
#include "CM_RepairLifecycle.h"

namespace CM
{
RepairCosting::RepairCosting(fs::FS& storage) : m_storage(storage), m_ready(false) {}

bool RepairCosting::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool RepairCosting::ready() const { return m_ready; }

bool RepairCosting::load(uint32_t repairId, RepairCostSummary& summary) const
{
    summary = RepairCostSummary();
    summary.repairId = repairId;
    if (!m_ready || repairId == 0UL || !repairExists(repairId)) return false;

    if (m_storage.exists(WireMovementsPath))
    {
        File file = m_storage.open(WireMovementsPath, FILE_READ);
        if (!file) return false;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            uint32_t lineRepairId = 0UL, mass = 0UL, price = 0UL;
            String type, status, currency, wireType;
            if (!findUnsigned(line, "repair_id", lineRepairId) || lineRepairId != repairId ||
                !findString(line, "type", type) || type != "WRITE_OFF" ||
                !findString(line, "status", status) || status != "CONFIRMED" ||
                !findUnsigned(line, "mass_g", mass) ||
                !findUnsigned(line, "price_per_kg_minor", price)) continue;

            const uint64_t lineCost =
                (static_cast<uint64_t>(mass) * static_cast<uint64_t>(price) + 500ULL) / 1000ULL;
            summary.wireCostMinor += lineCost;
            if (summary.wireLineCount < 0xFFFFU) ++summary.wireLineCount;

            findString(line, "wire_type", wireType);
            if (wireType == "CU")
            {
                summary.copperWireCostMinor += lineCost;
                summary.copperWireGrams += mass;
                if (summary.copperWireLineCount < 0xFFFFU) ++summary.copperWireLineCount;
            }
            else if (wireType == "AL")
            {
                summary.aluminiumWireCostMinor += lineCost;
                summary.aluminiumWireGrams += mass;
                if (summary.aluminiumWireLineCount < 0xFFFFU) ++summary.aluminiumWireLineCount;
            }
            else
            {
                summary.unknownWireCostMinor += lineCost;
                summary.unknownWireGrams += mass;
                if (summary.unknownWireLineCount < 0xFFFFU) ++summary.unknownWireLineCount;
            }

            if (findString(line, "currency", currency) && currency.length() == 3U)
                summary.currency = currency;
        }
        file.close();
    }

    if (m_storage.exists(MaterialUsagePath))
    {
        File file = m_storage.open(MaterialUsagePath, FILE_READ);
        if (!file) return false;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            uint32_t lineRepairId = 0UL;
            uint64_t cost = 0ULL;
            String currency;
            if (!findUnsigned(line, "repair_id", lineRepairId) || lineRepairId != repairId ||
                !findUnsigned64(line, "line_cost_minor", cost)) continue;
            summary.materialCostMinor += cost;
            if (summary.materialLineCount < 0xFFFFU) ++summary.materialLineCount;
            if (findString(line, "currency", currency) && currency.length() == 3U)
                summary.currency = currency;
        }
        file.close();
    }

    if (m_storage.exists(PricingPath))
    {
        File file = m_storage.open(PricingPath, FILE_READ);
        if (!file) return false;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            uint32_t lineRepairId = 0UL;
            if (!findUnsigned(line, "repair_id", lineRepairId) || lineRepairId != repairId) continue;
            if (!findUnsigned64(line, "labour_cost_minor", summary.labourCostMinor) ||
                !findUnsigned64(line, "client_price_minor", summary.clientPriceMinor))
            {
                file.close();
                return false;
            }
            String currency;
            if (!findString(line, "currency", currency) || currency.length() != 3U)
            {
                file.close();
                return false;
            }
            summary.currency = currency;
            String timestamp;
            if (!findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
            {
                file.close();
                return false;
            }
            summary.pricingUpdatedAt = timestamp;
            if (summary.pricingRevisionCount < 0xFFFFU) ++summary.pricingRevisionCount;
            else
            {
                file.close();
                return false;
            }
        }
        file.close();
    }

    summary.totalCostMinor = summary.wireCostMinor + summary.materialCostMinor + summary.labourCostMinor;
    summary.marginMinor = summary.clientPriceMinor > summary.totalCostMinor
                              ? summary.clientPriceMinor - summary.totalCostMinor
                              : 0ULL;
    return true;
}

bool RepairCosting::savePricing(uint32_t repairId,
                                uint64_t labourCostMinor,
                                uint64_t clientPriceMinor,
                                const String& currency,
                                const String& timestamp)
{
    if (!m_ready || repairId == 0UL || !repairExists(repairId) ||
        currency.length() != 3U || timestamp.length() < 10U)
        return false;

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_storage, repairId, repairOpen) || !repairOpen)
        return false;

    File file = m_storage.open(PricingPath, FILE_APPEND);
    if (!file) return false;
    char labour[24], client[24];
    snprintf(labour, sizeof(labour), "%llu", static_cast<unsigned long long>(labourCostMinor));
    snprintf(client, sizeof(client), "%llu", static_cast<unsigned long long>(clientPriceMinor));
    String line = F("{\"repair_id\":"); line += repairId;
    line += F(",\"labour_cost_minor\":"); line += labour;
    line += F(",\"client_price_minor\":"); line += client;
    line += F(",\"currency\":\""); line += jsonEscape(currency);
    line += F("\",\"timestamp\":\""); line += jsonEscape(timestamp); line += F("\"}\n");
    const size_t written = file.print(line);
    file.flush(); file.close();
    return written == line.length();
}

bool RepairCosting::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/repairs") && !m_storage.mkdir("/data/repairs")) return false;
    return true;
}

bool RepairCosting::findUnsigned(const String& line,const char* key,uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide);
    return true;
}

bool RepairCosting::findUnsigned64(const String& line,const char* key,uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int start = pos + marker.length();
    while (start < line.length() && line[start] == ' ') ++start;
    if (start >= line.length() || !isDigit(line[start])) return false;
    if (line[start] == '0' && start + 1 < line.length() && isDigit(line[start + 1]))
        return false;

    uint64_t parsed = 0ULL;
    int end = start;
    while (end < line.length() && isDigit(line[end]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[end] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++end;
    }

    while (end < line.length() && line[end] == ' ') ++end;
    if (end >= line.length() || (line[end] != ',' && line[end] != '}')) return false;

    value = parsed;
    return true;
}

bool RepairCosting::findString(const String& line,const char* key,String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int index = pos + marker.length();
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

String RepairCosting::jsonEscape(const String& value)
{
    String out; out.reserve(value.length()+8U);
    for(size_t i=0;i<value.length();++i){const char c=value[i];if(c=='\\')out+=F("\\\\");else if(c=='"')out+=F("\\\"");else if(c=='\n')out+=F("\\n");else if(c=='\r')out+=F("\\r");else if(static_cast<uint8_t>(c)>=0x20U)out+=c;}
    return out;
}
}
