#include "CM_CashPaymentIntegrityAudit.h"

#include <Arduino.h>

#include "CM_CashPaymentStore.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_RepairCosting.h"
#include "CM_RepairRegistry.h"

namespace CM
{
namespace
{
bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t size = file.size();
    if (size > 0xFFFFFFFFUL) return false;
    if (size == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(size - 1U)) || file.read() != '\n') return false;
    return file.seek(0U);
}

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    if (line[pos] == '0' && pos + 1U < line.length() && isDigit(line[pos + 1U])) return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit; ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed; return true;
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide); return true;
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (ch == '"') return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (ch == '\\')
        {
            if (pos >= line.length()) return false;
            const char escaped = line[pos++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
        }
        else
        {
            if (static_cast<uint8_t>(ch) < 0x20U) return false;
            value += ch;
        }
    }
    return false;
}

bool validMethod(const String& value)
{
    return value == "CASH" || value == "CARD" || value == "TRANSFER" || value == "OTHER";
}
}

bool CashPaymentIntegrityAudit::check(fs::FS& storage)
{
    uint32_t count = 0UL;
    return check(storage, count);
}

bool CashPaymentIntegrityAudit::check(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(CashPaymentStore::Path)) return true;

    CashPaymentStore payments(storage);
    RepairRegistry repairs(storage);
    RepairCosting costing(storage);
    if (!payments.begin() || !repairs.begin() || !costing.begin()) return false;

    File file = storage.open(CashPaymentStore::Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t id = 0UL, repairId = 0UL, clientId = 0UL, correctsId = 0UL;
        uint64_t amount = 0ULL;
        String kind, direction, currency, occurredAt, method;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned64(line, "amount_minor", amount) || amount == 0ULL ||
            !findString(line, "kind", kind) ||
            !findString(line, "direction", direction) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "occurred_at", occurredAt) ||
            occurredAt.length() < 10U || occurredAt.length() > 32U)
        {
            file.close(); return false;
        }
        if ((kind == "PAYMENT" && direction != "ADD") ||
            (kind == "CORRECTION" && direction != "ADD" && direction != "SUBTRACT") ||
            (kind != "PAYMENT" && kind != "CORRECTION"))
        {
            file.close(); return false;
        }
        if (line.indexOf(F("\"method\":")) >= 0 &&
            (!findString(line, "method", method) || !validMethod(method)))
        {
            file.close(); return false;
        }
        if (line.indexOf(F("\"corrects_event_id\":")) >= 0)
        {
            bool belongs = false;
            if (kind != "CORRECTION" ||
                !findUnsigned(line, "corrects_event_id", correctsId) ||
                correctsId == 0UL || correctsId >= id ||
                !payments.eventBelongsToRepair(correctsId, repairId, clientId, belongs) ||
                !belongs)
            {
                file.close(); return false;
            }
        }

        RepairIdentity identity;
        bool found = false;
        if (!repairs.loadRepairIdentity(repairId, identity, found) || !found ||
            identity.clientId != clientId)
        {
            file.close(); return false;
        }
        RepairCostSummary pricing;
        if (!costing.load(repairId, pricing) || pricing.currency != currency)
        {
            file.close(); return false;
        }

        previousId = id;
        if (recordCount == 0xFFFFFFFFUL) { file.close(); return false; }
        ++recordCount;
    }
    file.close();
    return true;
}
}
