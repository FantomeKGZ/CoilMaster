#include "CM_CashPaymentStore.h"

#include "CM_FlatJsonObjectValidator.h"

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

bool validKindDirection(const String& kind, const String& direction)
{
    if (kind == "PAYMENT") return direction == "ADD";
    if (kind == "CORRECTION") return direction == "ADD" || direction == "SUBTRACT";
    return false;
}

bool addChecked(uint64_t& target, uint64_t value)
{
    if (target > 0xFFFFFFFFFFFFFFFFULL - value) return false;
    target += value;
    return true;
}
}

CashPaymentStore::CashPaymentStore(fs::FS& storage) : m_storage(storage), m_ready(false) {}

bool CashPaymentStore::begin()
{
    m_ready = false;
    if (!ensureDirectory()) return false;
    if (m_storage.exists(Path) && !validateJournal()) return false;
    m_ready = true;
    return true;
}

bool CashPaymentStore::ready() const
{
    return m_ready;
}

bool CashPaymentStore::append(const NewCashEvent& event, uint32_t& eventId)
{
    eventId = 0UL;
    if (!ready() || event.repairId == 0UL || event.clientId == 0UL ||
        event.amountMinor == 0ULL || event.currency.length() != 3U ||
        event.occurredAt.length() < 10U || event.occurredAt.length() > 32U ||
        event.method.length() > 24U || event.comment.length() > 500U ||
        !validKindDirection(event.kind, event.direction) ||
        (event.kind == "PAYMENT" && event.correctsEventId != 0UL))
    {
        return false;
    }

    bool correctionFound = event.correctsEventId == 0UL;
    if (!analyzeAppendState(event.correctsEventId, eventId, correctionFound) ||
        !correctionFound)
    {
        eventId = 0UL;
        return false;
    }

    char amount[24];
    snprintf(amount, sizeof(amount), "%llu",
             static_cast<unsigned long long>(event.amountMinor));
    String line;
    line.reserve(900U);
    line = F("{\"cash_event_id\":"); line += eventId;
    line += F(",\"kind\":\""); line += event.kind;
    line += F("\",\"direction\":\""); line += event.direction;
    line += F("\",\"repair_id\":"); line += event.repairId;
    line += F(",\"client_id\":"); line += event.clientId;
    line += F(",\"amount_minor\":"); line += amount;
    line += F(",\"currency\":\""); line += jsonEscape(event.currency);
    line += F("\",\"occurred_at\":\""); line += jsonEscape(event.occurredAt); line += '"';
    if (event.method.length() > 0U)
    {
        line += F(",\"method\":\""); line += jsonEscape(event.method); line += '"';
    }
    if (event.correctsEventId != 0UL)
    {
        line += F(",\"corrects_event_id\":"); line += event.correctsEventId;
    }
    if (event.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(event.comment); line += '"';
    }
    line += F("}\n");

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }
    const size_t written = file.print(line);
    file.flush(); file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool CashPaymentStore::totalsForRepair(uint32_t repairId, CashRepairTotals& totals) const
{
    totals = CashRepairTotals();
    if (!ready() || repairId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }
    uint32_t previousId = 0UL;
    uint64_t added = 0ULL, subtracted = 0ULL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, lineRepair = 0UL;
        uint64_t amount = 0ULL;
        String kind, direction, currency, occurredAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "repair_id", lineRepair) || lineRepair == 0UL ||
            !findUnsigned64(line, "amount_minor", amount) || amount == 0ULL ||
            !findString(line, "kind", kind) || !findString(line, "direction", direction) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "occurred_at", occurredAt) || occurredAt.length() < 10U ||
            !validKindDirection(kind, direction))
        {
            file.close(); return false;
        }
        previousId = id;
        if (lineRepair != repairId) continue;
        if (!totals.currencySet) { totals.currency = currency; totals.currencySet = true; }
        else if (totals.currency != currency) { file.close(); return false; }
        if (totals.eventCount == 0xFFFFFFFFUL) { file.close(); return false; }
        ++totals.eventCount;
        if (direction == "ADD") { if (!addChecked(added, amount)) { file.close(); return false; } }
        else { if (!addChecked(subtracted, amount)) { file.close(); return false; } }
    }
    file.close();
    if (subtracted > added) return false;
    totals.paidMinor = added - subtracted;
    return true;
}

bool CashPaymentStore::appendRepairPageJson(String& json, uint32_t repairId,
                                             uint32_t cursor, uint8_t limit,
                                             uint16_t& count, uint32_t& nextCursor,
                                             bool& hasMore) const
{
    count = 0U; nextCursor = 0UL; hasMore = false;
    if (!ready() || repairId == 0UL || limit == 0U || limit > MaxPageSize) return false;
    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, lineRepair = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "repair_id", lineRepair) || lineRepair == 0UL)
        { file.close(); return false; }
        previousId = id;
        if (id <= cursor || lineRepair != repairId) continue;
        if (count >= limit) { hasMore = true; break; }
        if (count > 0U) json += ',';
        json += line;
        ++count; nextCursor = id;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}

bool CashPaymentStore::appendClientPageJson(String& json, uint32_t clientId,
                                             uint32_t cursor, uint8_t limit,
                                             uint16_t& count, uint32_t& nextCursor,
                                             bool& hasMore) const
{
    count = 0U; nextCursor = 0UL; hasMore = false;
    if (!ready() || clientId == 0UL || limit == 0U || limit > MaxPageSize) return false;
    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, lineClient = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "client_id", lineClient) || lineClient == 0UL)
        { file.close(); return false; }
        previousId = id;
        if (id <= cursor || lineClient != clientId) continue;
        if (count >= limit) { hasMore = true; break; }
        if (count > 0U) json += ',';
        json += line;
        ++count; nextCursor = id;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}

bool CashPaymentStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") && !m_storage.mkdir("/data/workshop")) return false;
    return true;
}

bool CashPaymentStore::analyzeAppendState(uint32_t correctionEventId,
                                          uint32_t& eventId,
                                          bool& correctionFound) const
{
    eventId = 1UL;
    correctionFound = correctionEventId == 0UL;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t id = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previous)
        {
            file.close();
            return false;
        }
        previous = id;
        if (id == correctionEventId) correctionFound = true;
    }
    file.close();

    if (previous == 0xFFFFFFFFUL) return false;
    eventId = previous + 1UL;
    return true;
}

bool CashPaymentStore::validateJournal() const
{
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file)) { if (file) file.close(); return false; }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, repairId = 0UL, clientId = 0UL, corrects = 0UL;
        uint64_t amount = 0ULL;
        String kind, direction, currency, occurredAt, value;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned64(line, "amount_minor", amount) || amount == 0ULL ||
            !findString(line, "kind", kind) || !findString(line, "direction", direction) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "occurred_at", occurredAt) || occurredAt.length() < 10U ||
            occurredAt.length() > 32U || !validKindDirection(kind, direction))
        { file.close(); return false; }
        const bool hasCorrects = line.indexOf(F("\"corrects_event_id\":")) >= 0;
        if ((kind == "PAYMENT" && hasCorrects) ||
            (hasCorrects && (!findUnsigned(line, "corrects_event_id", corrects) ||
                             corrects == 0UL || corrects >= id)))
        { file.close(); return false; }
        if (line.indexOf(F("\"method\":")) >= 0 &&
            (!findString(line, "method", value) || value.length() > 24U))
        { file.close(); return false; }
        if (line.indexOf(F("\"comment\":")) >= 0 &&
            (!findString(line, "comment", value) || value.length() > 500U))
        { file.close(); return false; }
        previousId = id;
    }
    file.close();
    return true;
}

bool CashPaymentStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide); return true;
}

bool CashPaymentStore::findUnsigned64(const String& line, const char* key, uint64_t& value)
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

bool CashPaymentStore::findString(const String& line, const char* key, String& value)
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

String CashPaymentStore::jsonEscape(const String& value)
{
    String out; out.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"') { out += '\\'; out += ch; }
        else if (ch == '\n') out += F("\\n");
        else if (ch == '\r') out += F("\\r");
        else if (ch == '\t') out += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) out += ch;
    }
    return out;
}
}
