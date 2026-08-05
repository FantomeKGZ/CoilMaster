#include "CM_ConductorSettings.h"

namespace CM
{
ConductorSettingsStore::ConductorSettingsStore(fs::FS& storage)
    : m_storage(storage), m_ready(false) {}

bool ConductorSettingsStore::begin()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/settings") && !m_storage.mkdir("/data/settings")) return false;
    m_ready = true;
    return true;
}

bool ConductorSettingsStore::load(ConversionSettings& settings) const
{
    settings = ConversionSettings();
    if (!m_ready || !m_storage.exists(SettingsPath)) return false;
    File file = m_storage.open(SettingsPath, FILE_READ);
    if (!file) return false;
    String line;
    while (file.available())
    {
        const String current = file.readStringUntil('\n');
        if (current.length() > 0U) line = current;
    }
    file.close();

    uint32_t alToCu = 0UL, cuToAl = 0UL, deviation = 0UL, maxStrands = 0UL;
    if (!findUnsigned(line, "aluminium_to_copper_permille", alToCu) ||
        !findUnsigned(line, "copper_to_aluminium_permille", cuToAl) ||
        !findUnsigned(line, "allowed_deviation_permille", deviation) ||
        !findUnsigned(line, "max_target_strands", maxStrands)) return false;

    if (alToCu < 100UL || alToCu > 3000UL || cuToAl < 100UL || cuToAl > 3000UL ||
        deviation < 1UL || deviation > 500UL || maxStrands < 1UL || maxStrands > 8UL) return false;

    settings.aluminiumToCopperPermille = static_cast<uint16_t>(alToCu);
    settings.copperToAluminiumPermille = static_cast<uint16_t>(cuToAl);
    settings.allowedDeviationPermille = static_cast<uint16_t>(deviation);
    settings.maxTargetStrands = static_cast<uint8_t>(maxStrands);
    return true;
}

bool ConductorSettingsStore::save(const ConversionSettings& settings)
{
    if (!m_ready || settings.aluminiumToCopperPermille < 100U ||
        settings.aluminiumToCopperPermille > 3000U ||
        settings.copperToAluminiumPermille < 100U ||
        settings.copperToAluminiumPermille > 3000U ||
        settings.allowedDeviationPermille < 1U ||
        settings.allowedDeviationPermille > 500U ||
        settings.maxTargetStrands < 1U || settings.maxTargetStrands > 8U) return false;

    m_storage.remove(TempPath);
    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file) return false;

    String line = F("{\"aluminium_to_copper_permille\":");
    line += settings.aluminiumToCopperPermille;
    line += F(",\"copper_to_aluminium_permille\":"); line += settings.copperToAluminiumPermille;
    line += F(",\"allowed_deviation_permille\":"); line += settings.allowedDeviationPermille;
    line += F(",\"max_target_strands\":"); line += settings.maxTargetStrands;
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length()) { m_storage.remove(TempPath); return false; }
    m_storage.remove(SettingsPath);
    return m_storage.rename(TempPath, SettingsPath);
}

bool ConductorSettingsStore::ready() const { return m_ready; }

bool ConductorSettingsStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0) return false;
    int index = start + marker.length();
    while (index < line.length() && line[index] == ' ') ++index;
    int end = index;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == index) return false;
    value = static_cast<uint32_t>(line.substring(index, end).toInt());
    return true;
}
}
