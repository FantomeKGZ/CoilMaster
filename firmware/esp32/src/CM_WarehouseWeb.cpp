#include "CM_WarehouseWeb.h"

namespace CM
{
namespace
{
void appendUnsigned64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    target += buffer;
}

bool normalizeWireType(const String& source, String& normalized)
{
    normalized = source;
    normalized.trim();
    normalized.toUpperCase();
    if (normalized == "CU" || normalized == "COPPER" || normalized == "МЕДЬ")
    {
        normalized = "CU";
        return true;
    }
    if (normalized == "AL" || normalized == "ALUMINIUM" ||
        normalized == "ALUMINUM" || normalized == "АЛЮМИНИЙ")
    {
        normalized = "AL";
        return true;
    }
    normalized = String();
    return false;
}
}

WarehouseWeb::WarehouseWeb(WebServer& server, WarehouseStore& store)
    : m_server(server), m_store(store)
{
}

void WarehouseWeb::begin()
{
    m_server.on("/api/warehouse/summary", HTTP_GET, [this]() { handleSummary(); });
    m_server.on("/api/warehouse/spools", HTTP_POST, [this]() { handleCreateSpool(); });
    m_server.on("/api/warehouse/price", HTTP_GET, [this]() { handleGetPrice(); });
    m_server.on("/api/warehouse/price", HTTP_POST, [this]() { handleSetPrice(); });
}

void WarehouseWeb::handleSummary()
{
    String month = m_server.hasArg("month") ? m_server.arg("month") : String();
    if (!validMonth(month))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"month_required_yyyy_mm\"}");
        return;
    }

    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }
    if (!m_store.loadSummary(month.c_str()))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_summary_read_failed\"}");
        return;
    }

    WarehousePrice price;
    bool priceConfigured = false;
    if (!m_store.loadWarehousePrice(price, priceConfigured))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_price_read_failed\"}");
        return;
    }

    String response;
    response.reserve(2400U);
    response = F("{\"month\":\"");
    response += month;
    response += F("\",\"total_remaining_g\":");
    response += m_store.totalRemainingGrams();
    response += F(",\"total_consumed_month_g\":");
    response += m_store.totalConsumedMonthGrams();
    response += F(",\"total_consumed_all_time_g\":");
    response += m_store.totalConsumedAllTimeGrams();
    response += F(",\"diameter_count\":");
    response += m_store.summaryCount();
    response += F(",\"price_configured\":");
    response += priceConfigured ? F("true") : F("false");
    response += F(",\"price_per_kg_minor\":");
    response += priceConfigured ? price.pricePerKgMinor : 0UL;
    response += F(",\"currency\":\"");
    response += price.currency;
    response += F("\",\"stock_value_minor\":");
    const uint64_t stockValue = priceConfigured
                                    ? static_cast<uint64_t>(m_store.totalRemainingGrams()) *
                                          price.pricePerKgMinor / 1000ULL
                                    : 0ULL;
    appendUnsigned64(response, stockValue);
    response += F(",\"diameters\":[");

    bool first = true;
    for (uint8_t i = 0U; i < m_store.summaryCount(); ++i)
    {
        WireStockSummary item;
        if (!m_store.summaryAt(i, item)) continue;
        if (!first) response += ',';
        first = false;

        response += F("{\"diameter_hundredths_mm\":");
        response += item.diameterHundredthsMm;
        response += F(",\"remaining_g\":");
        response += item.remainingGrams;
        response += F(",\"active_spools\":");
        response += item.activeSpoolCount;
        response += F(",\"consumed_month_g\":");
        response += item.consumedMonthGrams;
        response += F(",\"consumed_all_time_g\":");
        response += item.consumedAllTimeGrams;
        response += F(",\"remaining_value_minor\":");
        const uint64_t remainingValue = priceConfigured
                                            ? static_cast<uint64_t>(item.remainingGrams) *
                                                  price.pricePerKgMinor / 1000ULL
                                            : 0ULL;
        appendUnsigned64(response, remainingValue);
        response += '}';
    }

    response += F("]}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void WarehouseWeb::handleCreateSpool()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    uint32_t diameter = 0UL;
    uint32_t weight = 0UL;

    if (!parseUnsignedArg(m_server, "diameter_hundredths_mm", 1UL, 500UL, diameter))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_diameter_hundredths_mm\"}");
        return;
    }

    if (!parseUnsignedArg(m_server, "current_weight_g", 1UL, 1000000UL, weight))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_current_weight_g\"}");
        return;
    }

    String wireType;
    if (!m_server.hasArg("wire_type") ||
        !normalizeWireType(m_server.arg("wire_type"), wireType))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"wire_type_required_cu_or_al\"}");
        return;
    }

    NewWireSpool spool;
    spool.diameterHundredthsMm = static_cast<uint16_t>(diameter);
    spool.currentWeightGrams = weight;
    spool.wireType = wireType;
    spool.manufacturer = m_server.arg("manufacturer");
    spool.supplier = m_server.arg("supplier");
    spool.batch = m_server.arg("batch");
    spool.storageLocation = m_server.arg("storage_location");
    spool.comment = m_server.arg("comment");

    uint32_t spoolId = 0UL;
    if (!m_store.addSpool(spool, spoolId))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"spool_write_failed\"}");
        return;
    }

    String response = F("{\"created\":true,\"spool_id\":");
    response += spoolId;
    response += F(",\"diameter_hundredths_mm\":");
    response += spool.diameterHundredthsMm;
    response += F(",\"current_weight_g\":");
    response += spool.currentWeightGrams;
    response += F(",\"wire_type\":\"");
    response += spool.wireType;
    response += F("\"}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

void WarehouseWeb::handleGetPrice()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    WarehousePrice price;
    bool configured = false;
    if (!m_store.loadWarehousePrice(price, configured))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_price_read_failed\"}");
        return;
    }
    if (!configured)
    {
        m_server.send(200, "application/json; charset=utf-8",
                      "{\"configured\":false,\"price_per_kg_minor\":0,\"currency\":\"KGS\"}");
        return;
    }

    String response = F("{\"configured\":true,\"price_per_kg_minor\":");
    response += price.pricePerKgMinor;
    response += F(",\"currency\":\"");
    response += price.currency;
    response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void WarehouseWeb::handleSetPrice()
{
    uint32_t value = 0UL;
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    if (!parseUnsignedArg(m_server, "price_per_kg_minor", 1UL, 100000000UL, value))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_price_per_kg_minor\"}");
        return;
    }

    String currency = m_server.hasArg("currency") ? m_server.arg("currency") : String("KGS");
    currency.toUpperCase();
    if (currency.length() != 3U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_currency\"}");
        return;
    }

    WarehousePrice current;
    bool configured = false;
    if (!m_store.loadWarehousePrice(current, configured))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_price_read_failed\"}");
        return;
    }
    if (configured && current.pricePerKgMinor == value && current.currency == currency)
    {
        m_server.send(200, "application/json; charset=utf-8",
                      "{\"saved\":false,\"unchanged\":true,\"write_performed\":false}");
        return;
    }

    WarehousePrice price;
    price.pricePerKgMinor = value;
    price.currency = currency;
    if (!m_store.setWarehousePrice(price))
    {
        if (!m_store.ready())
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        else
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"price_write_failed\"}");
        return;
    }

    String response = F("{\"saved\":true,\"unchanged\":false,\"write_performed\":true,\"price_per_kg_minor\":");
    response += price.pricePerKgMinor;
    response += F(",\"currency\":\"");
    response += price.currency;
    response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool WarehouseWeb::validMonth(const String& month)
{
    if (month.length() != 7U || month[4] != '-') return false;
    for (uint8_t i = 0U; i < month.length(); ++i)
    {
        if (i == 4U) continue;
        if (!isDigit(month[i])) return false;
    }

    const uint8_t value = static_cast<uint8_t>((month[5] - '0') * 10 + (month[6] - '0'));
    return value >= 1U && value <= 12U;
}

bool WarehouseWeb::parseUnsignedArg(WebServer& server,
                                    const char* name,
                                    uint32_t minimum,
                                    uint32_t maximum,
                                    uint32_t& value)
{
    value = 0UL;
    if (name == nullptr || !server.hasArg(name) || minimum > maximum) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        if (parsed > maximum) return false;
    }

    if (parsed < minimum) return false;
    value = parsed;
    return true;
}
}
