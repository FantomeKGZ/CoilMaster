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

    SourceConductorSet source;
    source.material = sourceMaterial;
    uint32_t componentCount = 1UL;
    const bool multiComponent = m_server.hasArg("source_component_count");
    if (multiComponent &&
        !parseUnsignedArg(m_server, "source_component_count", 1UL,
                          MaxSourceConversionComponents, componentCount))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_source_bundle\"}");
        return;
    }
    source.componentCount = static_cast<uint8_t>(componentCount);
    for (uint8_t i = 0U; i < source.componentCount; ++i)
    {
        String diameterName;
        String strandsName;
        if (multiComponent)
        {
            diameterName = F("source_diameter_"); diameterName += i + 1U;
            diameterName += F("_hundredths_mm");
            strandsName = F("source_strands_"); strandsName += i + 1U;
        }
        else
        {
            diameterName = F("source_diameter_hundredths_mm");
            strandsName = F("source_parallel_strands");
        }
        uint32_t diameter = 0UL, strands = 0UL;
        if (!parseUnsignedArg(m_server, diameterName.c_str(), 1UL, 500UL, diameter) ||
            !parseUnsignedArg(m_server, strandsName.c_str(), 1UL, 12UL, strands))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_source_components\"}");
            return;
        }
        source.components[i].diameterHundredthsMm = static_cast<uint16_t>(diameter);
        source.components[i].parallelStrands = static_cast<uint8_t>(strands);
    }

    ConversionSettings settings;
    if (!m_warehouse.loadConversionSettings(settings))
    {
        if (!m_warehouse.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        }
        else
        {
            m_server.send(409, "application/json; charset=utf-8",
                          "{\"error\":\"calculator_not_configured\"}");
        }
        return;
    }

    KnownWireDiameter known[WarehouseMaxDiameters];
    const char* targetWireType = materialText(targetMaterial);
    uint8_t knownCount = 0U;
    if (!m_warehouse.loadKnownWireDiameters(
            targetWireType, known, WarehouseMaxDiameters, knownCount))
    {
        if (!m_warehouse.ready())
        {
            m_server.send(503, "application/json; charset=utf-8",
                          "{\"error\":\"warehouse_unavailable\"}");
        }
        else
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"wire_catalogue_read_failed\"}");
        }
        return;
    }
    if (knownCount == 0U)
    {
        String error = F("{\"error\":\"wire_catalogue_empty_for_material\",\"target_material\":\"");
        error += targetWireType;
        error += F("\"}");
        m_server.send(404, "application/json; charset=utf-8", error);
        return;
    }

    WireCandidate candidates[WarehouseMaxDiameters];
    for (uint8_t i = 0U; i < knownCount; ++i)
    {
        candidates[i].diameterHundredthsMm = known[i].diameterHundredthsMm;
        candidates[i].availableGrams = known[i].availableGrams;
        candidates[i].catalogKnown = true;
    }

    ConversionOption options[MaxRecommendedConversionOptions];
    const uint8_t optionCount = ConductorCalculator::findRecommendedOptions(
        source, targetMaterial, settings, candidates, knownCount, options);

    String response;
    response.reserve(3200U);
    response = F("{\"source_material\":\""); response += materialText(sourceMaterial);
    response += F("\",\"target_material\":\""); response += targetWireType;
    response += F("\",\"source_component_count\":"); response += source.componentCount;
    response += F(",\"source_components\":[");
    for (uint8_t i = 0U; i < source.componentCount; ++i)
    {
        if (i > 0U) response += ',';
        response += F("{\"diameter_hundredths_mm\":");
        response += source.components[i].diameterHundredthsMm;
        response += F(",\"parallel_strands\":");
        response += source.components[i].parallelStrands;
        response += '}';
    }
    response += F("],\"source_area_um2\":");
    response += ConductorCalculator::sourceSetAreaMicrometre2(source);
    response += F(",\"required_target_area_um2\":");
    response += ConductorCalculator::requiredTargetAreaMicrometre2(source, targetMaterial, settings);
    response += F(",\"catalogue_material\":\""); response += targetWireType;
    response += F("\",\"catalogue_diameter_count\":"); response += knownCount;
    response += F(",\"legacy_unknown_material_excluded\":true");
    response += F(",\"settings_source\":\"PERSISTED\"");
    response += F(",\"aluminium_to_copper_permille\":"); response += settings.aluminiumToCopperPermille;
    response += F(",\"copper_to_aluminium_permille\":"); response += settings.copperToAluminiumPermille;
    response += F(",\"allowed_deviation_permille\":"); response += settings.allowedDeviationPermille;
    response += F(",\"max_target_strands\":"); response += settings.maxTargetStrands;
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
    value = 0UL;
    if (name == nullptr || !server.hasArg(name)) return false;
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
    }

    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
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
