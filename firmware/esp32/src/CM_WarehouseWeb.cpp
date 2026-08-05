#include "CM_WarehouseWeb.h"

namespace CM
{
WarehouseWeb::WarehouseWeb(WebServer& server, WarehouseStore& store)
    : m_server(server), m_store(store)
{
}

void WarehouseWeb::begin()
{
    m_server.on("/api/warehouse/summary", HTTP_GET, [this]() { handleSummary(); });
    m_server.on("/api/warehouse/spools", HTTP_POST, [this]() { handleCreateSpool(); });
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

    if (!m_store.ready() || !m_store.loadSummary(month.c_str()))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    String response;
    response.reserve(2300U);
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
    uint32_t price = 0UL;

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

    if (m_server.hasArg("price_per_kg_minor") &&
        m_server.arg("price_per_kg_minor").length() > 0U &&
        !parseUnsignedArg(m_server, "price_per_kg_minor", 0UL, 100000000UL, price))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_price_per_kg_minor\"}");
        return;
    }

    NewWireSpool spool;
    spool.diameterHundredthsMm = static_cast<uint16_t>(diameter);
    spool.currentWeightGrams = weight;
    spool.pricePerKgMinor = price;
    spool.wireType = m_server.arg("wire_type");
    spool.manufacturer = m_server.arg("manufacturer");
    spool.supplier = m_server.arg("supplier");
    spool.batch = m_server.arg("batch");
    spool.storageLocation = m_server.arg("storage_location");
    spool.comment = m_server.arg("comment");

    uint32_t spoolId = 0UL;
    if (!m_store.addSpool(spool, spoolId))
    {
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
    response += F("}");
    m_server.send(201, "application/json; charset=utf-8", response);
}

bool WarehouseWeb::validMonth(const String& month)
{
    if (month.length() != 7U || month[4] != '-') return false;
    for (uint8_t i = 0U; i < month.length(); ++i)
    {
        if (i == 4U) continue;
        if (!isDigit(month[i])) return false;
    }

    const int value = month.substring(5).toInt();
    return value >= 1 && value <= 12;
}

bool WarehouseWeb::parseUnsignedArg(WebServer& server,
                                    const char* name,
                                    uint32_t minimum,
                                    uint32_t maximum,
                                    uint32_t& value)
{
    if (name == nullptr || !server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;

    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
    }

    const unsigned long parsed = strtoul(source.c_str(), nullptr, 10);
    if (parsed < minimum || parsed > maximum) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}
}
