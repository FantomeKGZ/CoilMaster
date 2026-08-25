#include "CM_MaterialRequestWarehousePendingStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool validUnit(const String& unit)
{
    return unit == "KG" || unit == "L" || unit == "PCS" ||
           unit == "M" || unit == "M2";
}

bool validCurrency(const String& currency)
{
    if (currency.length() != 3U) return false;
    for (size_t i = 0U; i < currency.length(); ++i)
    {
        if (currency[i] < 'A' || currency[i] > 'Z') return false;
    }
    return true;
}
}

bool MaterialRequestWarehousePending::valid() const
{
    if (transactionRef.length() < 8U || transactionRef.length() > 80U ||
        materialRequestId == 0UL || repairId == 0UL || warehouseItemId == 0UL ||
        quantityMilliUnits == 0UL || !validUnit(unit) || !validCurrency(currency) ||
        createdAt.length() < 10U || createdAt.length() > 32U ||
        comment.length() > 300U)
    {
        return false;
    }

    if (movementKind != "ISSUE" && movementKind != "RETURN" &&
        movementKind != "CORRECTION")
    {
        return false;
    }
    if (movementKind == "CORRECTION")
    {
        if (correctionDirection != "ADD" && correctionDirection != "REMOVE")
            return false;
    }
    else if (correctionDirection.length() != 0U)
    {
        return false;
    }

    if (sourceKind == "RUN_WIRE")
    {
        return movementKind == "ISSUE" && unit == "KG" &&
               sourceSessionId > 0UL && sourceRunId > 0UL &&
               (materialClass == "CU" || materialClass == "AL") &&
               wireDiameterHundredthsMm > 0U && wireDiameterHundredthsMm <= 500U;
    }

    if (sourceKind != "MANUAL_MATERIAL") return false;
    return sourceSessionId == 0UL && sourceRunId == 0UL &&
           materialClass.length() == 0U && wireDiameterHundredthsMm == 0U;
}

MaterialRequestWarehousePendingStore::MaterialRequestWarehousePendingStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MaterialRequestWarehousePendingStore::begin()
{
    m_ready = false;
    if (!ensureDirectory() || !recoverTemp()) return false;
    if (m_storage.exists(Path))
    {
        MaterialRequestWarehousePending pending;
        if (!loadPath(Path, pending)) return false;
    }
    m_ready = true;
    return true;
}

bool MaterialRequestWarehousePendingStore::ready() const
{
    return m_ready;
}

bool MaterialRequestWarehousePendingStore::hasPending() const
{
    return ready() && m_storage.exists(Path);
}

bool MaterialRequestWarehousePendingStore::save(
    const MaterialRequestWarehousePending& pending)
{
    if (!ready() || !pending.valid() || m_storage.exists(Path)) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;

    String line;
    line.reserve(1024U);
    line = F("{\"transaction_ref\":\""); line += jsonEscape(pending.transactionRef);
    line += F("\",\"material_request_id\":"); line += pending.materialRequestId;
    line += F(",\"repair_id\":"); line += pending.repairId;
    line += F(",\"warehouse_item_id\":"); line += pending.warehouseItemId;
    line += F(",\"movement_kind\":\""); line += pending.movementKind;
    line += F("\",\"source_kind\":\""); line += pending.sourceKind;
    line += F("\",\"correction_direction\":\""); line += pending.correctionDirection;
    line += F("\",\"quantity_milli_units\":"); line += pending.quantityMilliUnits;
    line += F(",\"unit\":\""); line += pending.unit;
    line += F("\",\"unit_cost_minor\":"); appendUint64(line, pending.unitCostMinor);
    line += F(",\"cost_amount_minor\":"); appendUint64(line, pending.costAmountMinor);
    line += F(",\"currency\":\""); line += pending.currency;
    line += F("\",\"created_at\":\""); line += jsonEscape(pending.createdAt);
    line += F("\",\"comment\":\""); line += jsonEscape(pending.comment);
    line += F("\",\"source_session_id\":"); line += pending.sourceSessionId;
    line += F(",\"source_run_id\":"); line += pending.sourceRunId;
    line += F(",\"material_class\":\""); line += pending.materialClass;
    line += F("\",\"wire_diameter_hundredths_mm\":");
    line += pending.wireDiameterHundredthsMm;
    line += F("}\n");

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

    MaterialRequestWarehousePending verified;
    if (!loadPath(TempPath, verified) ||
        verified.transactionRef != pending.transactionRef ||
        verified.materialRequestId != pending.materialRequestId ||
        verified.repairId != pending.repairId ||
        verified.warehouseItemId != pending.warehouseItemId ||
        verified.movementKind != pending.movementKind ||
        verified.sourceKind != pending.sourceKind ||
        verified.correctionDirection != pending.correctionDirection ||
        verified.quantityMilliUnits != pending.quantityMilliUnits ||
        verified.unit != pending.unit ||
        verified.unitCostMinor != pending.unitCostMinor ||
        verified.costAmountMinor != pending.costAmountMinor ||
        verified.currency != pending.currency ||
        verified.createdAt != pending.createdAt || verified.comment != pending.comment ||
        verified.sourceSessionId != pending.sourceSessionId ||
        verified.sourceRunId != pending.sourceRunId ||
        verified.materialClass != pending.materialClass ||
        verified.wireDiameterHundredthsMm != pending.wireDiameterHundredthsMm)
    {
        return false;
    }

    return m_storage.rename(TempPath, Path);
}

bool MaterialRequestWarehousePendingStore::load(
    MaterialRequestWarehousePending& pending, bool& found) const
{
    pending = MaterialRequestWarehousePending();
    found = false;
    if (!ready()) return false;
    if (!m_storage.exists(Path)) return true;
    if (!loadPath(Path, pending)) return false;
    found = true;
    return true;
}

bool MaterialRequestWarehousePendingStore::clear()
{
    if (!ready()) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
    if (!m_storage.exists(Path)) return true;
    return m_storage.remove(Path);
}

bool MaterialRequestWarehousePendingStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop")) return false;
    return true;
}

bool MaterialRequestWarehousePendingStore::recoverTemp()
{
    const bool mainExists = m_storage.exists(Path);
    const bool tempExists = m_storage.exists(TempPath);
    if (!tempExists) return true;

    MaterialRequestWarehousePending mainPending;
    if (mainExists && loadPath(Path, mainPending))
        return m_storage.remove(TempPath);

    MaterialRequestWarehousePending tempPending;
    if (!loadPath(TempPath, tempPending)) return false;
    if (mainExists) return false;
    return m_storage.rename(TempPath, Path);
}

bool MaterialRequestWarehousePendingStore::loadPath(
    const char* path, MaterialRequestWarehousePending& pending) const
{
    pending = MaterialRequestWarehousePending();
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t rawSize = file.size();
    if (rawSize == 0U || rawSize > 2048U)
    {
        file.close();
        return false;
    }
    String line = file.readStringUntil('\n');
    const bool extraData = file.available();
    file.close();
    if (extraData || line.length() == 0U || !FlatJsonObjectValidator::valid(line))
        return false;

    uint32_t wireDiameter = 0UL;
    if (!findString(line, "transaction_ref", pending.transactionRef) ||
        !findUnsigned(line, "material_request_id", pending.materialRequestId) ||
        !findUnsigned(line, "repair_id", pending.repairId) ||
        !findUnsigned(line, "warehouse_item_id", pending.warehouseItemId) ||
        !findString(line, "movement_kind", pending.movementKind) ||
        !findString(line, "source_kind", pending.sourceKind) ||
        !findString(line, "correction_direction", pending.correctionDirection) ||
        !findUnsigned(line, "quantity_milli_units", pending.quantityMilliUnits) ||
        !findString(line, "unit", pending.unit) ||
        !findUnsigned64(line, "unit_cost_minor", pending.unitCostMinor) ||
        !findUnsigned64(line, "cost_amount_minor", pending.costAmountMinor) ||
        !findString(line, "currency", pending.currency) ||
        !findString(line, "created_at", pending.createdAt) ||
        !findString(line, "comment", pending.comment) ||
        !findUnsigned(line, "source_session_id", pending.sourceSessionId) ||
        !findUnsigned(line, "source_run_id", pending.sourceRunId) ||
        !findString(line, "material_class", pending.materialClass) ||
        !findUnsigned(line, "wire_diameter_hundredths_mm", wireDiameter) ||
        wireDiameter > 0xFFFFUL)
    {
        return false;
    }
    pending.wireDiameterHundredthsMm = static_cast<uint16_t>(wireDiameter);
    return pending.valid();
}

bool MaterialRequestWarehousePendingStore::findUnsigned(
    const String& line, const char* key, uint32_t& value)
{
    uint64_t parsed = 0ULL;
    if (!findUnsigned64(line, key, parsed) || parsed > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool MaterialRequestWarehousePendingStore::findUnsigned64(
    const String& line, const char* key, uint64_t& value)
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

bool MaterialRequestWarehousePendingStore::findString(
    const String& line, const char* key, String& value)
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
        if (!escaped && ch == '"')
            return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (!escaped && ch == '\\')
        {
            escaped = true;
            continue;
        }
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

String MaterialRequestWarehousePendingStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"')
        {
            escaped += '\\';
            escaped += ch;
        }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}

void MaterialRequestWarehousePendingStore::appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    target += buffer;
}
}
