#include "CM_ConductorCalculatorWeb.h"

namespace CM
{
ConductorCalculatorWeb::ConductorCalculatorWeb(WebServer& server,
                                               WarehouseStore& warehouse)
    : m_server(server), m_warehouse(warehouse) {}

void ConductorCalculatorWeb::begin()
{
    m_server.on("/api/calculator/conductor", HTTP_GET,
                [this]() { handleCalculate(); });
}

void ConductorCalculatorWeb::handleCalculate()
{
    if (!m_warehouse.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"warehouse_unavailable\"}");
        return;
    }

    ConductorMaterial sourceMaterial;
    ConductorMaterial targetMaterial;
    if (!parseMaterial(m_server.arg("source_material"), sourceMaterial) ||
        !parseMaterial(m_server.arg("target_material"), targetMaterial) ||
        sourceMaterial == targetMaterial)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_material_conversion\"}");
        return;
    }

    uint32_t sourceDiameter = 0UL;
    uint32_t sourceStrands = 0UL;
    if (!parseUnsignedArg(m_server, "source_diameter_hundredths_mm", 1UL, 500UL,
                          sourceDiameter) ||
        !parseUnsignedArg(m_server, "source_parallel_strands", 1UL, 12UL,
                          sourceStrands))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_source_bundle\"}");
        return;
    }

    ConversionSettings settings;
    uint32_t parsed = 0UL;
    if (m_server.hasArg("al_to_cu_permille"))
    {
        if (!parseUnsignedArg(m_server, "al_to_cu_permille", 100UL, 3000UL, parsed))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_al_to_cu_permille\"}");
            return;
        }
        settings.aluminiumToCopperPermille = static_cast<uint16_t>(parsed);
    }
    if (m_server.hasArg("cu_to_al_permille"))
    {
        if (!parseUnsignedArg(m_server, "cu_to_al_permille", 100UL, 3000UL, parsed))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_cu_to_al_permille\"}");
            return;
        }
        settings.copperToAluminiumPermille = static_cast<uint16_t>(parsed);
    }
    if (m_server.hasArg("allowed_deviation_permille"))
    {
        if (!parseUnsignedArg(m_server, "allowed_deviation_permille", 1UL, 500UL, parsed))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_allowed_deviation_permille\"}");
            return;
        }
        settings.allowedDeviationPermille = static_cast<uint16_t>(parsed);
    }
    if (m_server.hasArg("max_target_strands"))
    {
        if (!parseUnsignedArg(m_server, "max_target_strands", 1UL, 8UL, parsed))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_max_target_strands\"}");
            return;
        }
        settings.maxTargetStrands = static_cast<uint8_t>(parsed);
    }
    if (m_server.hasArg("allow_mixed_diameters"))
    {
        const String value = m_server.arg("allow_mixed_diameters");
        settings.allowMixedDiameters = value != "0" && value != "false" && value != "FALSE";
    }

    KnownWireDiameter known[WarehouseMaxDiameters];
    const uint8_t knownCount =
        m_warehouse.loadKnownWireDiameters(known, WarehouseMaxDiameters);
    if (knownCount == 0U)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"wire_catalogue_empty\"}");
        return;
    }

    WireCandidate candidates[WarehouseMaxDiameters];
    for (uint8_t i = 0U; i < knownCount; ++i)
    {
        candidates[i].diameterHundredthsMm = known[i].diameterHundredthsMm;
        candidates[i].availableGrams = known[i].availableGrams;
        candidates[i].catalogKnown = true;
    }

    ConductorBundle source;
    source.material = sourceMaterial;
    source.diameterHundredthsMm = static_cast<uint16_t>(sourceDiameter);
    source.parallelStrands = static_cast<uint8_t>(sourceStrands);

    ConversionOption options[MaxRecommendedConversionOptions];
    const uint8_t optionCount = ConductorCalculator::findRecommendedOptions(
        source, targetMaterial, settings, candidates, knownCount, options);

    String response;
    response.reserve(3000U);
    response = F("{\"source_material\":\""); response += materialText(sourceMaterial);
    response += F("\",\"target_material\":\""); response += materialText(targetMaterial);
    response += F("\",\"source_diameter_hundredths_mm\":"); response += source.diameterHundredthsMm;
    response += F(",\"source_parallel_strands\":"); response += source.parallelStrands;
    response += F(",\"required_target_area_um2\":");
    response += ConductorCalculator::requiredTargetAreaMicrometre2(source, targetMaterial, settings);
    response += F(",\"catalogue_diameter_count\":"); response += knownCount;
    response += F(",\"mixed_diameters_enabled\":");
    response += settings.allowMixedDiameters ? F("true") : F("false");
    response += F(",\"recommendation_count\":"); response += optionCount;
    response += F(",\"recommendations\":[");

    for (uint8_t i = 0U; i < optionCount; ++i)
    {
        if (i > 0U) response += ',';
        const ConversionOption& option = options[i];
        response += F("{\"rank\":"); response += static_cast<uint8_t>(i + 1U);
        response += F(",\"parallel_strands\":"); response += option.targetParallelStrands;
        response += F(",\"target_area_um2\":"); response += option.targetAreaMicrometre2;
        response += F(",\"deviation_permille\":"); response += option.deviationPermille;
        response += F(",\"availability\":\""); response += availabilityText(option.availability);
        response += F("\",\"component_count\":"); response += option.componentCount;
        response += F(",\"components\":[");
        for (uint8_t componentIndex = 0U;
             componentIndex < option.componentCount;
             ++componentIndex)
        {
            if (componentIndex > 0U) response += ',';
            const ConversionComponent& component = option.components[componentIndex];
            response += F("{\"diameter_hundredths_mm\":");
            response += component.diameterHundredthsMm;
            response += F(",\"parallel_strands\":"); response += component.parallelStrands;
            response += F(",\"available_g\":"); response += component.availableGrams;
            response += '}';
        }
        response += F("]}");
    }

    response += F("]}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool ConductorCalculatorWeb::parseUnsignedArg(WebServer& server,
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

bool ConductorCalculatorWeb::parseMaterial(const String& value,
                                            ConductorMaterial& material)
{
    String normalized = value;
    normalized.toUpperCase();
    if (normalized == "CU" || normalized == "COPPER")
    {
        material = ConductorMaterial::Copper;
        return true;
    }
    if (normalized == "AL" || normalized == "ALUMINIUM" || normalized == "ALUMINUM")
    {
        material = ConductorMaterial::Aluminium;
        return true;
    }
    return false;
}

const char* ConductorCalculatorWeb::materialText(ConductorMaterial material)
{
    return material == ConductorMaterial::Copper ? "CU" : "AL";
}

const char* ConductorCalculatorWeb::availabilityText(ConversionAvailability availability)
{
    return availability == ConversionAvailability::InStock
               ? "IN_STOCK"
               : "PURCHASE_REQUIRED";
}
}
