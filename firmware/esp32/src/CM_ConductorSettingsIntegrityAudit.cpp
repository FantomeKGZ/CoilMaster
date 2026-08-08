#include "CM_ConductorSettingsIntegrityAudit.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr const char* SettingsPath = "/data/settings/conductor-calculator.ndjson";
constexpr const char* TempPath = "/data/settings/conductor-calculator.tmp";

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;

    int cursor = pos + marker.length();
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
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}
}

bool ConductorSettingsIntegrityAudit::check(fs::FS& storage)
{
    if (storage.exists(TempPath)) return false;
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
    if (line.length() < 2U || !line.startsWith("{") || !line.endsWith("}") ||
        extra.length() != 0U)
    {
        return false;
    }

    uint32_t alToCu = 0UL, cuToAl = 0UL, deviation = 0UL, maxStrands = 0UL;
    if (!findUnsigned(line, "aluminium_to_copper_permille", alToCu) ||
        !findUnsigned(line, "copper_to_aluminium_permille", cuToAl) ||
        !findUnsigned(line, "allowed_deviation_permille", deviation) ||
        !findUnsigned(line, "max_target_strands", maxStrands))
    {
        return false;
    }

    return alToCu >= 100UL && alToCu <= 3000UL &&
           cuToAl >= 100UL && cuToAl <= 3000UL &&
           deviation >= 1UL && deviation <= 500UL &&
           maxStrands >= 1UL && maxStrands <= 8UL;
}
}
