#include "CM_MaterialLedgerWeb.h"

namespace CM
{
void MaterialLedgerWeb::handleAdjust()
{
    if (!m_ledger.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"materials_unavailable\"}");
        return;
    }

    uint32_t materialId = 0UL;
    uint32_t addQuantity = 0UL;
    uint32_t newPrice = 0UL;
    if (!parseUnsigned(m_server, "material_id", 1UL, 0xFFFFFFFFUL, materialId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_material_id\"}");
        return;
    }

    if (m_server.hasArg("add_quantity_milli") &&
        m_server.arg("add_quantity_milli").length() > 0U &&
        !parseUnsigned(m_server, "add_quantity_milli", 0UL, 0xFFFFFFFFUL,
                       addQuantity))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_add_quantity\"}");
        return;
    }

    if (m_server.hasArg("new_price_per_unit_minor") &&
        m_server.arg("new_price_per_unit_minor").length() > 0U &&
        !parseUnsigned(m_server, "new_price_per_unit_minor", 0UL, 100000000UL,
                       newPrice))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_new_price\"}");
        return;
    }

    if (addQuantity == 0UL && newPrice == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"no_adjustment_requested\"}");
        return;
    }

    const String timestamp = m_server.arg("timestamp");
    if (timestamp.length() < 10U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"timestamp_required\"}");
        return;
    }

    String currency = m_server.hasArg("currency")
                          ? m_server.arg("currency")
                          : String("KGS");
    currency.toUpperCase();
    if (currency.length() != 3U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_currency\"}");
        return;
    }

    MaterialAdjustment adjustment;
    adjustment.materialId = materialId;
    adjustment.addQuantityMilli = addQuantity;
    adjustment.newPricePerUnitMinor = newPrice;
    adjustment.currency = currency;
    adjustment.timestamp = timestamp;
    adjustment.comment = m_server.arg("comment");

    MaterialAdjustmentResult result;
    if (!m_ledger.adjustMaterial(adjustment, result))
    {
        if (!m_ledger.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"materials_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"material_adjustment_failed\"}");
        }
        return;
    }

    String response = F("{\"updated\":true,\"adjustment_id\":");
    response += result.adjustmentId;
    response += F(",\"material_id\":"); response += materialId;
    response += F(",\"stock_quantity_milli\":");
    response += result.stockQuantityMilli;
    response += F(",\"price_per_unit_minor\":");
    response += result.pricePerUnitMinor;
    response += F(",\"currency\":\"");
    response += result.currency;
    response += F("\"}");
    m_server.send(200, "application/json; charset=utf-8", response);
}
}
