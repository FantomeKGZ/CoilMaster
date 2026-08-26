#include "CM_CashPaymentWeb.h"

namespace CM
{
CashPaymentWeb::CashPaymentWeb(WebServer& server,
                               RepairRegistry& repairs,
                               RepairCosting& costing,
                               CashPaymentStore& payments)
    : m_server(server), m_repairs(repairs), m_costing(costing), m_payments(payments)
{
}

void CashPaymentWeb::begin()
{
    m_server.on("/api/payments", HTTP_GET, [this]() { handleList(); });
    m_server.on("/api/payments", HTTP_POST, [this]() { handleCreate(); });
    m_server.on("/api/payments/balance", HTTP_GET, [this]() { handleBalance(); });
}

void CashPaymentWeb::handleList()
{
    if (!m_payments.ready())
    {
        m_server.send(503, "application/json", "{\"error\":\"payment_store_unavailable\"}");
        return;
    }
    const bool byRepair = m_server.hasArg("repair_id");
    const bool byClient = m_server.hasArg("client_id");
    if (byRepair == byClient)
    {
        m_server.send(400, "application/json", "{\"error\":\"exactly_one_payment_filter_required\"}");
        return;
    }
    uint32_t subjectId = 0UL, cursor = 0UL, limitWide = 20UL;
    if ((byRepair && !parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, subjectId)) ||
        (byClient && !parseUnsigned(m_server, "client_id", 1UL, 0xFFFFFFFFUL, subjectId)) ||
        (m_server.hasArg("cursor") &&
         !parseUnsigned(m_server, "cursor", 0UL, 0xFFFFFFFFUL, cursor)) ||
        (m_server.hasArg("limit") &&
         !parseUnsigned(m_server, "limit", 1UL, CashPaymentStore::MaxPageSize, limitWide)))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_payment_query\"}");
        return;
    }

    String response = F("{\"items\":[");
    uint16_t count = 0U;
    uint32_t nextCursor = 0UL;
    bool hasMore = false;
    const bool ok = byRepair
        ? m_payments.appendRepairPageJson(response, subjectId, cursor,
                                          static_cast<uint8_t>(limitWide), count,
                                          nextCursor, hasMore)
        : m_payments.appendClientPageJson(response, subjectId, cursor,
                                          static_cast<uint8_t>(limitWide), count,
                                          nextCursor, hasMore);
    if (!ok)
    {
        m_server.send(500, "application/json", "{\"error\":\"payment_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"has_more\":"); response += hasMore ? F("true") : F("false");
    response += F(",\"next_cursor\":");
    if (hasMore) response += nextCursor; else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void CashPaymentWeb::handleCreate()
{
    if (!m_server.hasArg("confirmed") || m_server.arg("confirmed") != "true")
    {
        m_server.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }
    if (!m_repairs.ready() || !m_costing.ready() || !m_payments.ready())
    {
        m_server.send(503, "application/json", "{\"error\":\"cash_runtime_unavailable\"}");
        return;
    }

    uint32_t repairId = 0UL;
    uint64_t amountMinor = 0ULL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId) ||
        !parseUnsigned64(m_server, "amount_minor", 1ULL, amountMinor) ||
        !m_server.hasArg("occurred_at") || !validTimestamp(m_server.arg("occurred_at")))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_payment_request\"}");
        return;
    }

    RepairIdentity identity;
    bool found = false;
    if (!m_repairs.loadRepairIdentity(repairId, identity, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_lookup_failed\"}");
        return;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"repair_not_found\"}");
        return;
    }

    RepairCostSummary pricing;
    if (!m_costing.load(repairId, pricing))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_pricing_unavailable\"}");
        return;
    }

    NewCashEvent event;
    event.repairId = repairId;
    event.clientId = identity.clientId;
    event.kind = m_server.hasArg("kind") ? m_server.arg("kind") : String("PAYMENT");
    event.direction = event.kind == "PAYMENT"
        ? String("ADD")
        : (m_server.hasArg("direction") ? m_server.arg("direction") : String());
    event.amountMinor = amountMinor;
    event.currency = m_server.hasArg("currency") ? m_server.arg("currency") : pricing.currency;
    event.occurredAt = m_server.arg("occurred_at");
    event.method = m_server.hasArg("method") ? m_server.arg("method") : String("CASH");
    event.comment = m_server.hasArg("comment") ? m_server.arg("comment") : String();
    if (m_server.hasArg("corrects_event_id") &&
        !parseUnsigned(m_server, "corrects_event_id", 1UL, 0xFFFFFFFFUL, event.correctsEventId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_correction_reference\"}");
        return;
    }

    if ((event.kind != "PAYMENT" && event.kind != "CORRECTION") ||
        (event.kind == "PAYMENT" && event.direction != "ADD") ||
        (event.kind == "CORRECTION" && event.direction != "ADD" && event.direction != "SUBTRACT") ||
        event.currency != pricing.currency || !validMethod(event.method))
    {
        m_server.send(409, "application/json", "{\"error\":\"payment_contract_mismatch\"}");
        return;
    }

    if (event.direction == "SUBTRACT")
    {
        CashRepairTotals current;
        if (!m_payments.totalsForRepair(repairId, current))
        {
            m_server.send(500, "application/json", "{\"error\":\"payment_total_read_failed\"}");
            return;
        }
        if (event.amountMinor > current.paidMinor)
        {
            m_server.send(409, "application/json", "{\"error\":\"correction_exceeds_paid_total\"}");
            return;
        }
    }

    uint32_t eventId = 0UL;
    if (!m_payments.append(event, eventId))
    {
        m_server.send(500, "application/json", "{\"error\":\"payment_write_failed\"}");
        return;
    }

    String response = F("{\"cash_event_id\":"); response += eventId;
    response += F(",\"repair_id\":"); response += repairId;
    response += F(",\"client_id\":"); response += identity.clientId;
    response += F(",\"kind\":\""); response += event.kind;
    response += F("\",\"direction\":\""); response += event.direction;
    response += F("\",\"amount_minor\":"); appendUint64(response, event.amountMinor);
    response += F(",\"currency\":\""); response += jsonEscape(event.currency); response += F("\"}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

void CashPaymentWeb::handleBalance()
{
    uint32_t repairId = 0UL;
    if (!parseUnsigned(m_server, "repair_id", 1UL, 0xFFFFFFFFUL, repairId))
    {
        m_server.send(400, "application/json", "{\"error\":\"invalid_repair_id\"}");
        return;
    }
    sendRepairBalance(repairId);
}

bool CashPaymentWeb::sendRepairBalance(uint32_t repairId)
{
    RepairIdentity identity;
    bool found = false;
    if (!m_repairs.loadRepairIdentity(repairId, identity, found))
    {
        m_server.send(500, "application/json", "{\"error\":\"repair_lookup_failed\"}");
        return false;
    }
    if (!found)
    {
        m_server.send(404, "application/json", "{\"error\":\"repair_not_found\"}");
        return false;
    }
    RepairCostSummary pricing;
    CashRepairTotals paid;
    if (!m_costing.load(repairId, pricing) || !m_payments.totalsForRepair(repairId, paid))
    {
        m_server.send(500, "application/json", "{\"error\":\"balance_read_failed\"}");
        return false;
    }
    if (paid.currencySet && paid.currency != pricing.currency)
    {
        m_server.send(409, "application/json", "{\"error\":\"balance_currency_mismatch\"}");
        return false;
    }
    const uint64_t debt = pricing.clientPriceMinor > paid.paidMinor
        ? pricing.clientPriceMinor - paid.paidMinor : 0ULL;
    const uint64_t credit = paid.paidMinor > pricing.clientPriceMinor
        ? paid.paidMinor - pricing.clientPriceMinor : 0ULL;

    String response = F("{\"repair_id\":"); response += repairId;
    response += F(",\"client_id\":"); response += identity.clientId;
    response += F(",\"motor_id\":"); response += identity.motorId;
    response += F(",\"charged_minor\":"); appendUint64(response, pricing.clientPriceMinor);
    response += F(",\"paid_minor\":"); appendUint64(response, paid.paidMinor);
    response += F(",\"debt_minor\":"); appendUint64(response, debt);
    response += F(",\"credit_minor\":"); appendUint64(response, credit);
    response += F(",\"payment_event_count\":"); response += paid.eventCount;
    response += F(",\"currency\":\""); response += jsonEscape(pricing.currency); response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
    return true;
}

bool CashPaymentWeb::parseUnsigned(WebServer& server, const char* name,
                                   uint32_t minimum, uint32_t maximum,
                                   uint32_t& value)
{
    value = 0UL;
    if (!server.hasArg(name)) return false;
    const String text = server.arg(name);
    if (text.length() == 0U || (text.length() > 1U && text[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < text.length(); ++i)
    {
        if (!isDigit(text[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(text[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed; return true;
}

bool CashPaymentWeb::parseUnsigned64(WebServer& server, const char* name,
                                     uint64_t minimum, uint64_t& value)
{
    value = 0ULL;
    if (!server.hasArg(name)) return false;
    const String text = server.arg(name);
    if (text.length() == 0U || (text.length() > 1U && text[0] == '0')) return false;
    uint64_t parsed = 0ULL;
    for (size_t i = 0U; i < text.length(); ++i)
    {
        if (!isDigit(text[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(text[i] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
    }
    if (parsed < minimum) return false;
    value = parsed; return true;
}

bool CashPaymentWeb::validTimestamp(const String& value)
{
    return value.length() >= 10U && value.length() <= 32U;
}

bool CashPaymentWeb::validMethod(const String& value)
{
    return value == "CASH" || value == "CARD" || value == "TRANSFER" || value == "OTHER";
}

String CashPaymentWeb::jsonEscape(const String& value)
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

void CashPaymentWeb::appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    target += buffer;
}
}
