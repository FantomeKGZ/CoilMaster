#include "CM_ConductorSettingsWeb.h"

namespace CM
{
ConductorSettingsWeb::ConductorSettingsWeb(WebServer& server, WarehouseStore& store)
    : m_server(server), m_store(store) {}

void ConductorSettingsWeb::begin()
{
    m_server.on("/api/calculator/settings", HTTP_GET, [this]() { handleGet(); });
    m_server.on("/api/calculator/settings", HTTP_POST, [this]() { handleSet(); });
}

void ConductorSettingsWeb::handleGet()
{
    ConversionSettings settings;
    const bool configured = m_store.loadConversionSettings(settings);
    String response = F("{\"configured\":");
    response += configured ? F("true") : F("false");
    response += F(",\"aluminium_to_copper_permille\":"); response += settings.aluminiumToCopperPermille;
    response += F(",\"copper_to_aluminium_permille\":"); response += settings.copperToAluminiumPermille;
    response += F(",\"allowed_deviation_permille\":"); response += settings.allowedDeviationPermille;
    response += F(",\"max_target_strands\":"); response += settings.maxTargetStrands;
    response += F(",\"allow_mixed_diameters\":"); response += settings.allowMixedDiameters ? F("true") : F("false");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void ConductorSettingsWeb::handleSet()
{
    uint32_t alToCu = 0UL, cuToAl = 0UL, deviation = 0UL, maxStrands = 0UL;
    if (!parseUnsigned(m_server,"aluminium_to_copper_permille",100UL,3000UL,alToCu) ||
        !parseUnsigned(m_server,"copper_to_aluminium_permille",100UL,3000UL,cuToAl) ||
        !parseUnsigned(m_server,"allowed_deviation_permille",1UL,500UL,deviation) ||
        !parseUnsigned(m_server,"max_target_strands",1UL,8UL,maxStrands))
    {
        m_server.send(400,"application/json; charset=utf-8","{\"error\":\"invalid_conversion_settings\"}");
        return;
    }

    ConversionSettings settings;
    settings.aluminiumToCopperPermille = static_cast<uint16_t>(alToCu);
    settings.copperToAluminiumPermille = static_cast<uint16_t>(cuToAl);
    settings.allowedDeviationPermille = static_cast<uint16_t>(deviation);
    settings.maxTargetStrands = static_cast<uint8_t>(maxStrands);
    if (m_server.hasArg("allow_mixed_diameters"))
    {
        const String value = m_server.arg("allow_mixed_diameters");
        settings.allowMixedDiameters = value != "0" && value != "false" && value != "FALSE";
    }

    if (!m_store.setConversionSettings(settings))
    {
        m_server.send(500,"application/json; charset=utf-8","{\"error\":\"settings_write_failed\"}");
        return;
    }
    m_server.send(200,"application/json; charset=utf-8","{\"saved\":true}");
}

bool ConductorSettingsWeb::parseUnsigned(WebServer& server,const char* name,uint32_t minValue,uint32_t maxValue,uint32_t& value)
{
    if (!server.hasArg(name)) return false;
    const String source = server.arg(name);
    if (source.length() == 0U) return false;
    for (size_t i=0U;i<source.length();++i) if (!isDigit(source[i])) return false;
    const unsigned long parsed = strtoul(source.c_str(),nullptr,10);
    if (parsed < minValue || parsed > maxValue) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}
}
