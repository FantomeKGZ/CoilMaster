#include "CM_ConductorSettings.h"

namespace CM
{
ConductorSettingsStore::ConductorSettingsStore(fs::FS& storage)
    : m_storage(storage), m_ready(false) {}

bool ConductorSettingsStore::begin()
{
    m_ready = false;
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/settings") && !m_storage.mkdir("/data/settings")) return false;
    if (!recoverFileSwap()) return false;
    m_ready = true;
    return true;
}

bool ConductorSettingsStore::load(ConversionSettings& settings) const
{
    settings = ConversionSettings();
    return ready() && m_storage.exists(SettingsPath) &&
           loadFromPath(SettingsPath, settings);
}

bool ConductorSettingsStore::save(const ConversionSettings& settings)
{
    if (!ready() || settings.aluminiumToCopperPermille < 100U ||
        settings.aluminiumToCopperPermille > 3000U ||
        settings.copperToAluminiumPermille < 100U ||
        settings.copperToAluminiumPermille > 3000U ||
        settings.allowedDeviationPermille < 1U ||
        settings.allowedDeviationPermille > 500U ||
        settings.maxTargetStrands < 1U || settings.maxTargetStrands > 8U ||
        !recoverFileSwap())
    {
        return false;
    }

    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
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
    if (written != line.length())
    {
        m_storage.remove(TempPath);
        return false;
    }

    ConversionSettings verified;
    if (!loadFromPath(TempPath, verified) ||
        verified.aluminiumToCopperPermille != settings.aluminiumToCopperPermille ||
        verified.copperToAluminiumPermille != settings.copperToAluminiumPermille ||
        verified.allowedDeviationPermille != settings.allowedDeviationPermille ||
        verified.maxTargetStrands != settings.maxTargetStrands)
    {
        m_storage.remove(TempPath);
        return false;
    }

    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    {
        m_storage.remove(TempPath);
        return false;
    }
    if (m_storage.exists(SettingsPath) && !m_storage.rename(SettingsPath, BackupPath))
    {
        m_storage.remove(TempPath);
        return false;
    }
    if (!m_storage.rename(TempPath, SettingsPath))
    {
        if (!m_storage.exists(SettingsPath) && m_storage.exists(BackupPath))
            m_storage.rename(BackupPath, SettingsPath);
        return false;
    }
    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    {
        m_ready = false;
        return false;
    }
    return true;
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

bool ConductorSettingsStore::recoverFileSwap()
{
    const bool mainExists = m_storage.exists(SettingsPath);
    const bool tempExists = m_storage.exists(TempPath);
    const bool backupExists = m_storage.exists(BackupPath);
    if (!tempExists && !backupExists) return true;

    ConversionSettings parsed;
    const bool mainValid = mainExists && loadFromPath(SettingsPath, parsed);
    if (mainValid)
    {
        if (tempExists && !m_storage.remove(TempPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }

    ConversionSettings tempSettings;
    ConversionSettings backupSettings;
    const bool tempValid = tempExists && loadFromPath(TempPath, tempSettings);
    const bool backupValid = backupExists && loadFromPath(BackupPath, backupSettings);

    if (mainExists && !m_storage.remove(SettingsPath)) return false;

    // Backup is the last committed conversion configuration. A valid temp next
    // to it means save() prepared a candidate and rotated the old main, but the
    // candidate was not yet committed. Restore backup first so a brownout cannot
    // silently apply new conversion ratios or limits that the operator never saw
    // complete successfully.
    if (backupExists)
    {
        if (!backupValid) return false;
        if (tempExists && !m_storage.remove(TempPath)) return false;
        return m_storage.rename(BackupPath, SettingsPath);
    }

    // No backup means there was no previous committed settings file. A complete
    // verified temp is therefore an interrupted first write and may be promoted.
    if (tempValid) return m_storage.rename(TempPath, SettingsPath);
    if (tempExists && !m_storage.remove(TempPath)) return false;

    return false;
}

bool ConductorSettingsStore::loadFromPath(const char* path,
                                          ConversionSettings& settings) const
{
    settings = ConversionSettings();
    if (path == nullptr || !m_storage.exists(path)) return false;
    File file = m_storage.open(path, FILE_READ);
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
        !findUnsigned(line, "max_target_strands", maxStrands) ||
        alToCu < 100UL || alToCu > 3000UL ||
        cuToAl < 100UL || cuToAl > 3000UL ||
        deviation < 1UL || deviation > 500UL ||
        maxStrands < 1UL || maxStrands > 8UL)
    {
        return false;
    }

    settings.aluminiumToCopperPermille = static_cast<uint16_t>(alToCu);
    settings.copperToAluminiumPermille = static_cast<uint16_t>(cuToAl);
    settings.allowedDeviationPermille = static_cast<uint16_t>(deviation);
    settings.maxTargetStrands = static_cast<uint8_t>(maxStrands);
    return true;
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
