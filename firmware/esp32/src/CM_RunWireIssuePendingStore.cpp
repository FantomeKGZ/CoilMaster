#include "CM_RunWireIssuePendingStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool RunWireIssuePending::valid() const
{
    return transactionRef.length() >= 8U && transactionRef.length() <= 80U &&
           materialRequestId != 0UL && repairId != 0UL && warehouseItemId != 0UL &&
           sourceSessionId != 0UL && sourceRunId != 0UL && spoolId != 0UL &&
           consumedGrams != 0UL && spoolWeightBeforeGrams > consumedGrams &&
           spoolWeightAfterGrams == spoolWeightBeforeGrams - consumedGrams &&
           ledgerQuantityMilli != 0UL && unitCostMinor != 0ULL &&
           costAmountMinor != 0ULL && currency.length() == 3U &&
           (materialClass == "CU" || materialClass == "AL") &&
           wireDiameterHundredthsMm > 0U && wireDiameterHundredthsMm <= 500U &&
           createdAt.length() >= 10U && createdAt.length() <= 32U &&
           comment.length() <= 300U;
}

RunWireIssuePendingStore::RunWireIssuePendingStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool RunWireIssuePendingStore::begin()
{
    m_ready = false;
    if (!ensureDirectory() || !recoverTemp()) return false;
    if (m_storage.exists(Path))
    {
        RunWireIssuePending pending;
        if (!loadPath(Path, pending)) return false;
    }
    m_ready = true;
    return true;
}

bool RunWireIssuePendingStore::ready() const { return m_ready; }
bool RunWireIssuePendingStore::hasPending() const
{
    return ready() && m_storage.exists(Path);
}

bool RunWireIssuePendingStore::save(const RunWireIssuePending& pending)
{
    if (!ready() || !pending.valid() || m_storage.exists(Path)) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;

    String line;
    line.reserve(1200U);
    line = F("{\"transaction_ref\":\""); line += jsonEscape(pending.transactionRef);
    line += F("\",\"material_request_id\":"); line += pending.materialRequestId;
    line += F(",\"repair_id\":"); line += pending.repairId;
    line += F(",\"warehouse_item_id\":"); line += pending.warehouseItemId;
    line += F(",\"source_session_id\":"); line += pending.sourceSessionId;
    line += F(",\"source_run_id\":"); line += pending.sourceRunId;
    line += F(",\"spool_id\":"); line += pending.spoolId;
    line += F(",\"consumed_grams\":"); line += pending.consumedGrams;
    line += F(",\"spool_weight_before_g\":"); line += pending.spoolWeightBeforeGrams;
    line += F(",\"spool_weight_after_g\":"); line += pending.spoolWeightAfterGrams;
    line += F(",\"ledger_quantity_milli\":"); line += pending.ledgerQuantityMilli;
    line += F(",\"unit_cost_minor\":"); appendUint64(line, pending.unitCostMinor);
    line += F(",\"cost_amount_minor\":"); appendUint64(line, pending.costAmountMinor);
    line += F(",\"currency\":\""); line += pending.currency;
    line += F("\",\"material_class\":\""); line += pending.materialClass;
    line += F("\",\"wire_diameter_hundredths_mm\":"); line += pending.wireDiameterHundredthsMm;
    line += F(",\"created_at\":\""); line += jsonEscape(pending.createdAt);
    line += F("\",\"comment\":\""); line += jsonEscape(pending.comment);
    line += F("\"}\n");

    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length()) return false;

    RunWireIssuePending verified;
    if (!loadPath(TempPath, verified) ||
        verified.transactionRef != pending.transactionRef ||
        verified.materialRequestId != pending.materialRequestId ||
        verified.sourceSessionId != pending.sourceSessionId ||
        verified.sourceRunId != pending.sourceRunId ||
        verified.spoolId != pending.spoolId ||
        verified.spoolWeightBeforeGrams != pending.spoolWeightBeforeGrams ||
        verified.spoolWeightAfterGrams != pending.spoolWeightAfterGrams ||
        verified.ledgerQuantityMilli != pending.ledgerQuantityMilli)
    {
        return false;
    }
    return m_storage.rename(TempPath, Path);
}

bool RunWireIssuePendingStore::load(RunWireIssuePending& pending, bool& found) const
{
    pending = RunWireIssuePending();
    found = false;
    if (!ready()) return false;
    if (!m_storage.exists(Path)) return true;
    if (!loadPath(Path, pending)) return false;
    found = true;
    return true;
}

bool RunWireIssuePendingStore::clear()
{
    if (!ready()) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
    return !m_storage.exists(Path) || m_storage.remove(Path);
}

bool RunWireIssuePendingStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") && !m_storage.mkdir("/data/workshop")) return false;
    return true;
}

bool RunWireIssuePendingStore::recoverTemp()
{
    if (!m_storage.exists(TempPath)) return true;
    if (m_storage.exists(Path))
    {
        RunWireIssuePending mainPending;
        if (!loadPath(Path, mainPending)) return false;
        return m_storage.remove(TempPath);
    }
    RunWireIssuePending tempPending;
    if (!loadPath(TempPath, tempPending)) return false;
    return m_storage.rename(TempPath, Path);
}

bool RunWireIssuePendingStore::loadPath(const char* path, RunWireIssuePending& pending) const
{
    pending = RunWireIssuePending();
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t rawSize = file.size();
    if (rawSize == 0U || rawSize > 3072U)
    {
        file.close();
        return false;
    }
    String line = file.readStringUntil('\n');
    const bool extraData = file.available();
    file.close();
    if (extraData || line.length() == 0U || !FlatJsonObjectValidator::valid(line)) return false;

    uint32_t diameter = 0UL;
    if (!findString(line, "transaction_ref", pending.transactionRef) ||
        !findUnsigned(line, "material_request_id", pending.materialRequestId) ||
        !findUnsigned(line, "repair_id", pending.repairId) ||
        !findUnsigned(line, "warehouse_item_id", pending.warehouseItemId) ||
        !findUnsigned(line, "source_session_id", pending.sourceSessionId) ||
        !findUnsigned(line, "source_run_id", pending.sourceRunId) ||
        !findUnsigned(line, "spool_id", pending.spoolId) ||
        !findUnsigned(line, "consumed_grams", pending.consumedGrams) ||
        !findUnsigned(line, "spool_weight_before_g", pending.spoolWeightBeforeGrams) ||
        !findUnsigned(line, "spool_weight_after_g", pending.spoolWeightAfterGrams) ||
        !findUnsigned(line, "ledger_quantity_milli", pending.ledgerQuantityMilli) ||
        !findUnsigned64(line, "unit_cost_minor", pending.unitCostMinor) ||
        !findUnsigned64(line, "cost_amount_minor", pending.costAmountMinor) ||
        !findString(line, "currency", pending.currency) ||
        !findString(line, "material_class", pending.materialClass) ||
        !findUnsigned(line, "wire_diameter_hundredths_mm", diameter) || diameter > 0xFFFFUL ||
        !findString(line, "created_at", pending.createdAt) ||
        !findString(line, "comment", pending.comment))
    {
        return false;
    }
    pending.wireDiameterHundredthsMm = static_cast<uint16_t>(diameter);
    return pending.valid();
}

bool RunWireIssuePendingStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t parsed = 0ULL;
    if (!findUnsigned64(line, key, parsed) || parsed > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool RunWireIssuePendingStore::findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (UINT64_MAX - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

bool RunWireIssuePendingStore::findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (!escaped && ch == '"') return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (!escaped && ch == '\\') { escaped = true; continue; }
        if (escaped)
        {
            if (ch == '"' || ch == '\\') value += ch;
            else if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else return false;
            escaped = false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

String RunWireIssuePendingStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"') { escaped += '\\'; escaped += ch; }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}

void RunWireIssuePendingStore::appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    target += buffer;
}
}
