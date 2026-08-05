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
    response += F(",\"items\":[");

    for (uint8_t i = 0U; i < m_store.summaryCount(); ++i)
    {
        WireStockSummary item;
        if (!m_store.summaryAt(i, item)) continue;
        if (i > 0U) response += ',';

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
}
