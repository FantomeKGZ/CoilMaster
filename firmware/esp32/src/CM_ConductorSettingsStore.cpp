#include "CM_WarehouseStore.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool findBoolean(const String& line,
                 const char* key,
                 bool& value,
                 bool& found)
{
    value = false;
    found = false;
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0) return true;
    if (line.indexOf(marker, position + marker.length()) >= 0) return false;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (line.substring(cursor, cursor + 4) == "true")
    {
        cursor += 4;
        value = true;
    }
    else if (line.substring(cursor, cursor + 5) == "false")
    {
        cursor += 5;
        value = false;
    }
    else
    {
        return false;
    }

    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}'))
    {
        return false;
    }
    found = true;
    return true;
}

bool sameSettings(const ConversionSettings& left,
                  const ConversionSettings& right)
{
    return left.aluminiumToCopperPermille == right.aluminiumToCopperPermille &&
           left.copperToAluminiumPermille == right.copperToAluminiumPermille &&
           left.allowedDeviationPermille == right.allowedDeviationPermille &&
           left.maxTargetStrands == right.maxTargetStrands &&
           left.allowMixedDiameters == right.allowMixedDiameters;
}
}

bool WarehouseStore::setConversionSettings(const ConversionSettings& settings)
{
    if (!m_ready || settings.aluminiumToCopperPermille < 100U ||
        settings.aluminiumToCopperPermille > 3000U ||
        settings.copperToAluminiumPermille < 100U ||
        settings.copperToAluminiumPermille > 3000U ||
        settings.allowedDeviationPermille < 1U ||
        settings.allowedDeviationPermille > 500U ||
        settings.maxTargetStrands < 1U || settings.maxTargetStrands > 8U)
    {
        return false;
    }

    if (!m_storage.exists("/data/settings") && !m_storage.mkdir("/data/settings"))
    {
        return false;
    }
    if (!recoverConversionSettingsFileSwap()) return false;
    if (m_storage.exists(ConversionSettingsTempPath) &&
        !m_storage.remove(ConversionSettingsTempPath))
    {
        return false;
    }

    File file = m_storage.open(ConversionSettingsTempPath, FILE_WRITE);
    if (!file) return false;

    String line;
    line.reserve(220U);
    line = F("{\"aluminium_to_copper_permille\":");
    line += settings.aluminiumToCopperPermille;
    line += F(",\"copper_to_aluminium_permille\":");
    line += settings.copperToAluminiumPermille;
    line += F(",\"allowed_deviation_permille\":");
    line += settings.allowedDeviationPermille;
    line += F(",\"max_target_strands\":");
    line += settings.maxTargetStrands;
    line += F(",\"allow_mixed_diameters\":");
    line += settings.allowMixedDiameters ? F("true") : F("false");
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_storage.remove(ConversionSettingsTempPath);
        return false;
    }

    ConversionSettings verified;
    if (!loadConversionSettingsFromPath(ConversionSettingsTempPath, verified) ||
        !sameSettings(verified, settings))
    {
        m_storage.remove(ConversionSettingsTempPath);
        return false;
    }

    if (m_storage.exists(ConversionSettingsBackupPath) &&
        !m_storage.remove(ConversionSettingsBackupPath))
    {
        m_storage.remove(ConversionSettingsTempPath);
        return false;
    }
    const bool hadMain = m_storage.exists(ConversionSettingsPath);
    if (hadMain &&
        !m_storage.rename(ConversionSettingsPath, ConversionSettingsBackupPath))
    {
        m_storage.remove(ConversionSettingsTempPath);
        return false;
    }
    if (!m_storage.rename(ConversionSettingsTempPath, ConversionSettingsPath))
    {
        if (hadMain && !m_storage.exists(ConversionSettingsPath) &&
            m_storage.exists(ConversionSettingsBackupPath))
        {
            m_storage.rename(ConversionSettingsBackupPath, ConversionSettingsPath);
        }
        return false;
    }

    ConversionSettings committed;
    if (!loadConversionSettingsFromPath(ConversionSettingsPath, committed) ||
        !sameSettings(committed, settings))
    {
        return false;
    }
    if (hadMain && m_storage.exists(ConversionSettingsBackupPath) &&
        !m_storage.remove(ConversionSettingsBackupPath))
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool WarehouseStore::loadConversionSettings(ConversionSettings& settings) const
{
    settings = ConversionSettings();
    if (!m_ready || !recoverConversionSettingsFileSwap() ||
        !m_storage.exists(ConversionSettingsPath))
    {
        return false;
    }
    return loadConversionSettingsFromPath(ConversionSettingsPath, settings);
}

bool WarehouseStore::recoverConversionSettingsFileSwap() const
{
    const bool mainExists = m_storage.exists(ConversionSettingsPath);
    const bool tempExists = m_storage.exists(ConversionSettingsTempPath);
    const bool backupExists = m_storage.exists(ConversionSettingsBackupPath);
    if (!tempExists && !backupExists) return true;

    ConversionSettings mainSettings;
    if (mainExists &&
        loadConversionSettingsFromPath(ConversionSettingsPath, mainSettings))
    {
        if (tempExists && !m_storage.remove(ConversionSettingsTempPath)) return false;
        if (backupExists && !m_storage.remove(ConversionSettingsBackupPath)) return false;
        return true;
    }

    ConversionSettings backupSettings;
    const bool backupValid = backupExists &&
        loadConversionSettingsFromPath(ConversionSettingsBackupPath, backupSettings);
    ConversionSettings tempSettings;
    const bool tempValid = tempExists &&
        loadConversionSettingsFromPath(ConversionSettingsTempPath, tempSettings);

    if (mainExists && !m_storage.remove(ConversionSettingsPath)) return false;

    // A backup proves there was a previously committed main. Prefer it over the
    // prepared temp because power may have failed after main -> backup but before
    // temp -> main. Never finish that uncommitted edit automatically after reboot.
    if (backupExists)
    {
        if (!backupValid) return false;
        if (tempExists && !m_storage.remove(ConversionSettingsTempPath)) return false;
        return m_storage.rename(ConversionSettingsBackupPath,
                                ConversionSettingsPath);
    }

    // With no backup, a valid temp can only be an interrupted first write.
    if (tempValid)
        return m_storage.rename(ConversionSettingsTempPath, ConversionSettingsPath);
    if (tempExists && !m_storage.remove(ConversionSettingsTempPath)) return false;
    return !mainExists;
}

bool WarehouseStore::loadConversionSettingsFromPath(
    const char* path,
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
    const String line = file.readStringUntil('\n');
    const String extra = file.readStringUntil('\n');
    const bool terminated = !file.available();
    file.close();
    if (!terminated || extra.length() != 0U ||
        !FlatJsonObjectValidator::valid(line))
    {
        return false;
    }

    uint32_t alToCu = 0UL;
    uint32_t cuToAl = 0UL;
    uint32_t deviation = 0UL;
    uint32_t maxStrands = 0UL;
    bool allowMixed = true;
    bool allowMixedFound = false;
    if (!findUnsigned(line, "aluminium_to_copper_permille", alToCu) ||
        !findUnsigned(line, "copper_to_aluminium_permille", cuToAl) ||
        !findUnsigned(line, "allowed_deviation_permille", deviation) ||
        !findUnsigned(line, "max_target_strands", maxStrands) ||
        !findBoolean(line, "allow_mixed_diameters", allowMixed, allowMixedFound) ||
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
    settings.allowMixedDiameters = allowMixedFound ? allowMixed : true;
    return true;
}
}
