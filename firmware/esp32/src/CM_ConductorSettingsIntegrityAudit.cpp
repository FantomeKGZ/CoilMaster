#include "CM_ConductorSettingsIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr const char* SettingsPath = "/data/settings/conductor.json";
constexpr const char* LegacySettingsPath = "/data/settings/conductor-calculator.ndjson";
constexpr const char* LegacyTempPath = "/data/settings/conductor-calculator.tmp";
constexpr const char* LegacyBackupPath = "/data/settings/conductor-calculator.bak";

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;

    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool optionalBooleanValid(const String& line, const char* key)
{
    const String marker = String("\"") + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0) return true;
    if (line.indexOf(marker, position + marker.length()) >= 0) return false;

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (line.substring(cursor, cursor + 4) == "true") cursor += 4;
    else if (line.substring(cursor, cursor + 5) == "false") cursor += 5;
    else return false;

    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    return cursor < line.length() &&
           (line[cursor] == ',' || line[cursor] == '}');
}
}

bool ConductorSettingsIntegrityAudit::check(fs::FS& storage)
{
    // The production API contract is /data/settings/conductor.json. Presence of
    // the older experimental store is ambiguous and must not be called stable.
    if (storage.exists(LegacySettingsPath) ||
        storage.exists(LegacyTempPath) ||
        storage.exists(LegacyBackupPath))
    {
        return false;
    }

    if (!storage.exists(SettingsPath)) return true;

    File file = storage.open(SettingsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const String line = file.readStringUntil('\n');
    const String extra = file.readStringUntil('\n');
    file.close();
    if (!FlatJsonObjectValidator::valid(line) || extra.length() != 0U)
    {
        return false;
    }

    uint32_t alToCu = 0UL, cuToAl = 0UL, deviation = 0UL, maxStrands = 0UL;
    if (!findUnsigned(line, "aluminium_to_copper_permille", alToCu) ||
        !findUnsigned(line, "copper_to_aluminium_permille", cuToAl) ||
        !findUnsigned(line, "allowed_deviation_permille", deviation) ||
        !findUnsigned(line, "max_target_strands", maxStrands) ||
        !optionalBooleanValid(line, "allow_mixed_diameters"))
    {
        return false;
    }

    return alToCu >= 100UL && alToCu <= 3000UL &&
           cuToAl >= 100UL && cuToAl <= 3000UL &&
           deviation >= 1UL && deviation <= 500UL &&
           maxStrands >= 1UL && maxStrands <= 8UL;
}
}
