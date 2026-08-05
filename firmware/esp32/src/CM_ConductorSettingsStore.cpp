#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::setConversionSettings(const ConversionSettings& settings)
{
    if (!m_ready || settings.aluminiumToCopperPermille == 0U ||
        settings.copperToAluminiumPermille == 0U ||
        settings.allowedDeviationPermille == 0U ||
        settings.maxTargetStrands == 0U)
    {
        return false;
    }

    if (!m_storage.exists("/data/settings") && !m_storage.mkdir("/data/settings"))
    {
        return false;
    }

    File file = m_storage.open(ConversionSettingsPath, FILE_WRITE);
    if (!file) return false;

    String line;
    line.reserve(180U);
    line = F("{\"aluminium_to_copper_permille\":");
    line += settings.aluminiumToCopperPermille;
    line += F(",\"copper_to_aluminium_permille\":");
    line += settings.copperToAluminiumPermille;
    line += F(",\"allowed_deviation_permille\":");
    line += settings.allowedDeviationPermille;
    line += F(",\"max_target_strands\":");
    line += settings.maxTargetStrands;
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    return written == line.length();
}

bool WarehouseStore::loadConversionSettings(ConversionSettings& settings) const
{
    settings = ConversionSettings();
    if (!m_ready || !m_storage.exists(ConversionSettingsPath))
    {
        return false;
    }

    File file = m_storage.open(ConversionSettingsPath, FILE_READ);
    if (!file) return false;
    const String line = file.readStringUntil('\n');
    file.close();

    uint32_t alToCu = 0UL;
    uint32_t cuToAl = 0UL;
    uint32_t deviation = 0UL;
    uint32_t maxStrands = 0UL;
    if (!findUnsigned(line, "aluminium_to_copper_permille", alToCu) ||
        !findUnsigned(line, "copper_to_aluminium_permille", cuToAl) ||
        !findUnsigned(line, "allowed_deviation_permille", deviation) ||
        !findUnsigned(line, "max_target_strands", maxStrands) ||
        alToCu > 3000UL || cuToAl > 3000UL || deviation > 500UL ||
        maxStrands > 8UL || alToCu < 100UL || cuToAl < 100UL ||
        deviation < 1UL || maxStrands < 1UL)
    {
        settings = ConversionSettings();
        return false;
    }

    settings.aluminiumToCopperPermille = static_cast<uint16_t>(alToCu);
    settings.copperToAluminiumPermille = static_cast<uint16_t>(cuToAl);
    settings.allowedDeviationPermille = static_cast<uint16_t>(deviation);
    settings.maxTargetStrands = static_cast<uint8_t>(maxStrands);
    return true;
}
}
