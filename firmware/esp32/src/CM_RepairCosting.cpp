#include "CM_RepairCosting.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_RepairLifecycle.h"
#include "CM_WarehouseMovementIntegrityAudit.h"
#include "CM_WarehouseWriteOffRecord.h"

namespace CM
{
namespace
{
bool addChecked64(uint64_t& target, uint64_t value)
{
    if (target > 0xFFFFFFFFFFFFFFFFULL - value) return false;
    target += value;
    return true;
}

bool addChecked32(uint32_t& target, uint32_t value)
{
    if (target > 0xFFFFFFFFUL - value) return false;
    target += value;
    return true;
}

bool acceptCurrency(const String& value, String& current, bool& set)
{
    if (value.length() != 3U) return false;
    if (!set)
    {
        current = value;
        set = true;
        return true;
    }
    return current == value;
}

bool storageDirectoryReady(fs::FS& storage, const char* path)
{
    if (path == nullptr || !storage.exists(path)) return false;
    File probe = storage.open(path);
    if (!probe) return false;
    const bool directory = probe.isDirectory();
    probe.close();
    return directory;
}
}

RepairCosting::RepairCosting(fs::FS& storage) : m_storage(storage), m_ready(false) {}

bool RepairCosting::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool RepairCosting::ready() const
{
    return m_ready &&
           storageDirectoryReady(m_storage, "/data/repairs") &&
           storageDirectoryReady(m_storage, "/data/materials") &&
           storageDirectoryReady(m_storage, "/data/warehouse");
}

bool RepairCosting::load(uint32_t repairId, RepairCostSummary& summary) const
{
    summary = RepairCostSummary();
    summary.repairId = repairId;
    if (!ready() || repairId == 0UL || !repairExists(repairId)) return false;

    // This is the authoritative whole-journal transaction/provenance validation.
    // Costing deliberately consumes only CONFIRMED records after this pass instead
    // of maintaining a second legacy-only parser that could drift from warehouse
    // persistence semantics.
    if (!WarehouseMovementIntegrityAudit::check(m_storage)) return false;

    bool currencySet = false;

    if (m_storage.exists(WireMovementsPath))
    {
        File file = m_storage.open(WireMovementsPath, FILE_READ);
        if (!file) return false;

        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;

            WarehouseWriteOffRecord record;
            if (!WarehouseWriteOffRecordCodec::parse(line, record))
            {
                file.close();
                return false;
            }
            if (record.status != "CONFIRMED" || record.repairId != repairId) continue;

            const uint64_t product = static_cast<uint64_t>(record.massGrams) *
                                     static_cast<uint64_t>(record.pricePerKgMinor);
            if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL)
            {
                file.close();
                return false;
            }
            const uint64_t lineCost = (product + 500ULL) / 1000ULL;

            if (!addChecked64(summary.wireCostMinor, lineCost) ||
                summary.wireLineCount == 0xFFFFU ||
                !acceptCurrency(record.currency, summary.currency, currencySet))
            {
                file.close();
                return false;
            }
            ++summary.wireLineCount;

            if (record.wireType == "CU")
            {
                if (!addChecked64(summary.copperWireCostMinor, lineCost) ||
                    !addChecked32(summary.copperWireGrams, record.massGrams) ||
                    summary.copperWireLineCount == 0xFFFFU)
                {
                    file.close();
                    return false;
                }
                ++summary.copperWireLineCount;
            }
            else if (record.wireType == "AL")
            {
                if (!addChecked64(summary.aluminiumWireCostMinor, lineCost) ||
                    !addChecked32(summary.aluminiumWireGrams, record.massGrams) ||
                    summary.aluminiumWireLineCount == 0xFFFFU)
                {
                    file.close();
                    return false;
                }
                ++summary.aluminiumWireLineCount;
            }
            else
            {
                if (!addChecked64(summary.unknownWireCostMinor, lineCost) ||
                    !addChecked32(summary.unknownWireGrams, record.massGrams) ||
                    summary.unknownWireLineCount == 0xFFFFU)
                {
                    file.close();
                    return false;
                }
                ++summary.unknownWireLineCount;
            }
        }
        file.close();
    }

    if (m_storage.exists(MaterialUsagePath))
    {
        File file = m_storage.open(MaterialUsagePath, FILE_READ);
        if (!file) return false;

        uint32_t previousUsageId = 0UL;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;

            uint32_t usageId = 0UL;
            uint32_t lineRepairId = 0UL;
            uint32_t materialId = 0UL;
            uint32_t quantity = 0UL;
            uint32_t unitPrice = 0UL;
            uint64_t lineCost = 0ULL;
            String currency, timestamp, comment;
            const bool hasComment = line.indexOf(F("\"comment\":")) >= 0;

            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "usage_id", usageId) || usageId == 0UL ||
                usageId <= previousUsageId ||
                !findUnsigned(line, "repair_id", lineRepairId) || lineRepairId == 0UL ||
                !findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
                !findUnsigned(line, "quantity_milli", quantity) || quantity == 0UL ||
                !findUnsigned(line, "price_per_unit_minor", unitPrice) || unitPrice == 0UL ||
                !findUnsigned64(line, "line_cost_minor", lineCost) ||
                !findString(line, "currency", currency) || currency.length() != 3U ||
                !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
                (hasComment && !findString(line, "comment", comment)))
            {
                file.close();
                return false;
            }
            previousUsageId = usageId;

            const uint64_t product = static_cast<uint64_t>(quantity) *
                                     static_cast<uint64_t>(unitPrice);
            if (product > 0xFFFFFFFFFFFFFFFFULL - 500ULL ||
                lineCost != (product + 500ULL) / 1000ULL)
            {
                file.close();
                return false;
            }

            if (lineRepairId == repairId)
            {
                if (!addChecked64(summary.materialCostMinor, lineCost) ||
                    summary.materialLineCount == 0xFFFFU ||
                    !acceptCurrency(currency, summary.currency, currencySet))
                {
                    file.close();
                    return false;
                }
                ++summary.materialLineCount;
            }
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
            if (line.length() == 0U) continue;

            uint32_t lineRepairId = 0UL;
            uint64_t labour = 0ULL;
            uint64_t client = 0ULL;
            String currency, timestamp;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "repair_id", lineRepairId) || lineRepairId == 0UL ||
                !findUnsigned64(line, "labour_cost_minor", labour) ||
                !findUnsigned64(line, "client_price_minor", client) ||
                !findString(line, "currency", currency) || currency.length() != 3U ||
                !findString(line, "timestamp", timestamp) || timestamp.length() < 10U)
            {
                file.close();
                return false;
            }

            if (lineRepairId != repairId) continue;
            if (summary.pricingRevisionCount == 0xFFFFU ||
                !acceptCurrency(currency, summary.currency, currencySet))
            {
                file.close();
                return false;
            }
            ++summary.pricingRevisionCount;
            summary.labourCostMinor = labour;
            summary.clientPriceMinor = client;
            summary.pricingUpdatedAt = timestamp;
        }
        file.close();
    }

    uint64_t total = summary.wireCostMinor;
    if (!addChecked64(total, summary.materialCostMinor) ||
        !addChecked64(total, summary.labourCostMinor))
    {
        return false;
    }
    summary.totalCostMinor = total;
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
    if (!ready() || repairId == 0UL || !repairExists(repairId) ||
        currency.length() != 3U || timestamp.length() < 10U)
        return false;

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_storage, repairId, repairOpen) || !repairOpen)
        return false;

    RepairCostSummary current;
    if (!load(repairId, current) || currency != current.currency ||
        current.pricingRevisionCount == 0xFFFFU ||
        (labourCostMinor == current.labourCostMinor &&
         clientPriceMinor == current.clientPriceMinor))
    {
        return false;
    }

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
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
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
