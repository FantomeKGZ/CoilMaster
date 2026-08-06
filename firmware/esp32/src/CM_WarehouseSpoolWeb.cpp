#include "CM_WarehouseWeb.h"
#include "CM_ConductorCalculatorWeb.h"
#include "CM_ConductorSettingsWeb.h"
#include "CM_MaterialLedger.h"
#include "CM_MaterialLedgerWeb.h"
#include "CM_RepairRegistry.h"
#include "CM_RepairRegistryWeb.h"
#include "CM_MotorSimilarityWeb.h"

namespace CM
{
void WarehouseWeb::beginSpoolList()
{
    m_server.on("/api/warehouse/spools", HTTP_GET, [this]() { handleListSpools(); });
    beginWriteOff();

    static ConductorCalculatorWeb calculatorWeb(m_server, m_store);
    static ConductorSettingsWeb conductorSettingsWeb(m_server, m_store);
    static MaterialLedger materialLedger(m_store.storage());
    static MaterialLedgerWeb materialLedgerWeb(m_server, materialLedger);
    static RepairRegistry repairRegistry(m_store.storage());
    static RepairRegistryWeb repairRegistryWeb(m_server, repairRegistry);
    static MotorSimilarityWeb motorSimilarityWeb(m_server, repairRegistry);

    calculatorWeb.begin();
    conductorSettingsWeb.begin();
    materialLedger.begin();
    materialLedgerWeb.begin();
    repairRegistry.begin();
    repairRegistryWeb.begin();
    motorSimilarityWeb.begin();
}

void WarehouseWeb::handleListSpools()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    uint16_t diameter = 0U;
    if (m_server.hasArg("diameter_hundredths_mm") &&
        m_server.arg("diameter_hundredths_mm").length() > 0U)
    {
        uint32_t parsed = 0UL;
        if (!parseUnsignedArg(m_server,"diameter_hundredths_mm",1UL,500UL,parsed))
        {
            m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_diameter_hundredths_mm\"}");
            return;
        }
        diameter = static_cast<uint16_t>(parsed);
    }

    String material = m_server.hasArg("material") ? m_server.arg("material") : String("ALL");
    material.toUpperCase();
    if (material != "ALL" && material != "CU" && material != "AL" && material != "UNKNOWN")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_material_filter\"}");
        return;
    }

    String response;
    response.reserve(4096U);
    response = F("{\"diameter_hundredths_mm\":"); response += diameter;
    response += F(",\"material_filter\":\""); response += material;
    response += F("\",\"items\":[");
    uint16_t count = 0U;
    if (!m_store.appendActiveSpoolsJson(response, diameter, material.c_str(), count))
    {
        m_server.send(500,"application/json; charset=utf-8","{\"error\":\"spool_read_failed\"}");
        return;
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"legacy_unknown_material_included\":");
    response += (material == "ALL" || material == "UNKNOWN") ? F("true") : F("false");
    response += '}';
    m_server.send(200,"application/json; charset=utf-8",response);
}
}
