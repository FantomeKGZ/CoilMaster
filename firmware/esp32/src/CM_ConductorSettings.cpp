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
    if (!ready() || !m_storage.exists(SettingsPath)) return false;
    File file = m_storage.open(SettingsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    String line;
    bool recordSeen = false;
    while (file.available())
    {
        const String current = file.readStringUntil('\n');
        if (current.length() == 0U) continue;
        if (recordSeen)
        {
            file.close();
            return false;
        }
        line = current;
        recordSeen = true;
    }
    file.close();

    if (!recordSeen || line.length() < 2U ||
        !line.startsWith("{") || !line.endsWith("}"))
    {
        return false;
    }

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
    if (!ready() || settings.aluminiumToCopperPermille < 100U ||
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

bool ConductorSettingsStore::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open("/data/settings", FILE_READ);
    if (!directory) return false;
    const bool available = directory.isDirectory();
    directory.close();
    return available;
}

bool ConductorSettingsStore::findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;

    int index = start + marker.length();
    while (index < line.length() && line[index] == ' ') ++index;
    if (index >= line.length() || !isDigit(line[index])) return false;
    if (line[index] == '0' && index + 1 < line.length() && isDigit(line[index + 1]))
        return false;

    uint32_t parsed = 0UL;
    while (index < line.length() && isDigit(line[index]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[index] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++index;
    }

    while (index < line.length() && line[index] == ' ') ++index;
    if (index >= line.length() || (line[index] != ',' && line[index] != '}')) return false;

    value = parsed;
    return true;
}
}
