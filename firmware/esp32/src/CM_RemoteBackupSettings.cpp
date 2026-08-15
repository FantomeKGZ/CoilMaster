#include "CM_RemoteBackupSettings.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
constexpr uint32_t SchemaVersion = 2UL;

bool safeHostCharacter(char ch)
{
    return isAlphaNumeric(ch) || ch == '.' || ch == '-';
}

bool safeUserCharacter(char ch)
{
    return isAlphaNumeric(ch) || ch == '.' || ch == '-' || ch == '_' ||
           ch == '@';
}

bool safeDirectoryCharacter(char ch)
{
    return isAlphaNumeric(ch) || ch == '/' || ch == '.' || ch == '-' ||
           ch == '_';
}

bool allCharactersValid(const String& value,
                        bool (*predicate)(char),
                        size_t maxLength)
{
    if (value.length() == 0U || value.length() > maxLength) return false;
    for (size_t i = 0U; i < value.length(); ++i)
    {
        if (!predicate(value[i])) return false;
    }
    return true;
}

bool validPassword(const String& password)
{
    if (password.length() == 0U || password.length() > 64U) return false;
    for (size_t i = 0U; i < password.length(); ++i)
    {
        const uint8_t value = static_cast<uint8_t>(password[i]);
        if (value < 0x21U || value > 0x7EU) return false;
    }
    return true;
}

bool validDateKey(uint32_t value)
{
    if (value == 0UL) return true;
    const uint16_t year = static_cast<uint16_t>(value / 10000UL);
    const uint8_t month = static_cast<uint8_t>((value / 100UL) % 100UL);
    const uint8_t day = static_cast<uint8_t>(value % 100UL);
    if (year < 2000U || year > 2099U || month < 1U || month > 12U ||
        day < 1U) return false;
    static const uint8_t daysPerMonth[] =
        {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint8_t maximumDay = daysPerMonth[month - 1U];
    if (month == 2U && year % 4U == 0U) maximumDay = 29U;
    return day <= maximumDay;
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;

    int cursor = start + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() &&
        isDigit(line[cursor + 1]))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}'))
    {
        return false;
    }
    value = parsed;
    return true;
}

bool findBoolean(const String& line, const char* key, bool& value)
{
    value = false;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    int cursor = start + marker.length();
    if (line.substring(cursor, cursor + 4) == "true")
    {
        value = true;
        cursor += 4;
    }
    else if (line.substring(cursor, cursor + 5) == "false")
    {
        cursor += 5;
    }
    else
    {
        return false;
    }
    return cursor < line.length() &&
           (line[cursor] == ',' || line[cursor] == '}');
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    const int valueStart = start + marker.length();
    const int valueEnd = line.indexOf('"', valueStart);
    if (valueEnd < 0 || valueEnd + 1 >= line.length() ||
        (line[valueEnd + 1] != ',' && line[valueEnd + 1] != '}'))
    {
        return false;
    }
    value = line.substring(valueStart, valueEnd);
    return true;
}

char hexDigit(uint8_t value)
{
    return value < 10U ? static_cast<char>('0' + value)
                       : static_cast<char>('A' + value - 10U);
}

bool fromHexDigit(char source, uint8_t& value)
{
    if (source >= '0' && source <= '9')
    {
        value = static_cast<uint8_t>(source - '0');
        return true;
    }
    if (source >= 'A' && source <= 'F')
    {
        value = static_cast<uint8_t>(source - 'A' + 10);
        return true;
    }
    return false;
}

String encodeHex(const String& source)
{
    String encoded;
    encoded.reserve(source.length() * 2U);
    for (size_t i = 0U; i < source.length(); ++i)
    {
        const uint8_t value = static_cast<uint8_t>(source[i]);
        encoded += hexDigit(static_cast<uint8_t>(value >> 4U));
        encoded += hexDigit(static_cast<uint8_t>(value & 0x0FU));
    }
    return encoded;
}

bool decodeHex(const String& source, String& decoded)
{
    decoded = String();
    if (source.length() == 0U || source.length() > 128U ||
        (source.length() % 2U) != 0U)
    {
        return false;
    }
    decoded.reserve(source.length() / 2U);
    for (size_t i = 0U; i < source.length(); i += 2U)
    {
        uint8_t high = 0U, low = 0U;
        if (!fromHexDigit(source[i], high) ||
            !fromHexDigit(source[i + 1U], low))
        {
            return false;
        }
        decoded += static_cast<char>((high << 4U) | low);
    }
    return true;
}
}

RemoteBackupSettingsStore::RemoteBackupSettingsStore(fs::FS& storage)
    : m_storage(storage), m_ready(false) {}

bool RemoteBackupSettingsStore::begin()
{
    m_ready = false;
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists(Directory) && !m_storage.mkdir(Directory)) return false;
    if (!recoverFileSwap()) return false;

    if (m_storage.exists(SettingsPath))
    {
        RemoteBackupSettings settings;
        if (!loadFromPath(SettingsPath, settings)) return false;
    }
    m_ready = true;
    return true;
}

bool RemoteBackupSettingsStore::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open(Directory, FILE_READ);
    if (!directory) return false;
    const bool available = directory.isDirectory();
    directory.close();
    return available;
}

bool RemoteBackupSettingsStore::load(RemoteBackupSettings& settings,
                                     bool& configured) const
{
    settings = RemoteBackupSettings();
    configured = false;
    if (!ready()) return false;
    if (!m_storage.exists(SettingsPath)) return true;
    if (!loadFromPath(SettingsPath, settings)) return false;
    configured = true;
    return true;
}

bool RemoteBackupSettingsStore::save(const RemoteBackupSettings& settings)
{
    if (!ready() || !valid(settings) || !recoverFileSwap()) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;

    String line;
    line.reserve(520U);
    line = F("{\"schema\":2,\"enabled\":");
    line += settings.enabled ? F("true") : F("false");
    line += F(",\"host\":\""); line += settings.host;
    line += F("\",\"port\":"); line += settings.port;
    line += F(",\"username\":\""); line += settings.username;
    line += F("\",\"password_hex\":\""); line += encodeHex(settings.password);
    line += F("\",\"remote_directory\":\""); line += settings.remoteDirectory;
    line += F("\",\"retention_count\":"); line += settings.retentionCount;
    line += F(",\"schedule_enabled\":");
    line += settings.scheduleEnabled ? F("true") : F("false");
    line += F(",\"schedule_hour\":");
    line += static_cast<unsigned>(settings.scheduleHour);
    line += F(",\"schedule_minute\":");
    line += static_cast<unsigned>(settings.scheduleMinute);
    line += F(",\"last_scheduled_date\":"); line += settings.lastScheduledDate;
    line += F("}\n");

    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file) return false;
    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_storage.remove(TempPath);
        return false;
    }

    RemoteBackupSettings verified;
    if (!loadFromPath(TempPath, verified) ||
        verified.enabled != settings.enabled || verified.host != settings.host ||
        verified.port != settings.port || verified.username != settings.username ||
        verified.password != settings.password ||
        verified.remoteDirectory != settings.remoteDirectory ||
        verified.retentionCount != settings.retentionCount ||
        verified.scheduleEnabled != settings.scheduleEnabled ||
        verified.scheduleHour != settings.scheduleHour ||
        verified.scheduleMinute != settings.scheduleMinute ||
        verified.lastScheduledDate != settings.lastScheduledDate)
    {
        m_storage.remove(TempPath);
        return false;
    }

    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    {
        m_storage.remove(TempPath);
        return false;
    }
    if (m_storage.exists(SettingsPath) &&
        !m_storage.rename(SettingsPath, BackupPath))
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

bool RemoteBackupSettingsStore::valid(const RemoteBackupSettings& settings)
{
    if (!allCharactersValid(settings.host, safeHostCharacter, 63U) ||
        !allCharactersValid(settings.username, safeUserCharacter, 32U) ||
        !validPassword(settings.password) ||
        !allCharactersValid(settings.remoteDirectory,
                            safeDirectoryCharacter,
                            96U) ||
        !settings.remoteDirectory.startsWith("/") ||
        settings.remoteDirectory.indexOf("..") >= 0 || settings.port == 0U ||
        settings.retentionCount < 1U || settings.retentionCount > 30U ||
        settings.scheduleHour > 23U || settings.scheduleMinute > 59U ||
        !validDateKey(settings.lastScheduledDate))
    {
        return false;
    }
    return true;
}

bool RemoteBackupSettingsStore::recoverFileSwap()
{
    const bool mainExists = m_storage.exists(SettingsPath);
    const bool tempExists = m_storage.exists(TempPath);
    const bool backupExists = m_storage.exists(BackupPath);
    if (!tempExists && !backupExists) return true;

    RemoteBackupSettings parsed;
    if (mainExists && loadFromPath(SettingsPath, parsed))
    {
        if (tempExists && !m_storage.remove(TempPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }

    RemoteBackupSettings tempSettings, backupSettings;
    const bool tempValid = tempExists && loadFromPath(TempPath, tempSettings);
    const bool backupValid = backupExists && loadFromPath(BackupPath, backupSettings);
    if (mainExists && !m_storage.remove(SettingsPath)) return false;
    if (tempValid)
    {
        if (!m_storage.rename(TempPath, SettingsPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }
    if (tempExists && !m_storage.remove(TempPath)) return false;
    if (backupValid) return m_storage.rename(BackupPath, SettingsPath);
    return false;
}

bool RemoteBackupSettingsStore::loadFromPath(
    const char* path,
    RemoteBackupSettings& settings) const
{
    settings = RemoteBackupSettings();
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

    uint32_t schema = 0UL, port = 0UL, retention = 0UL;
    bool enabled = false;
    String host, username, passwordHex, password, remoteDirectory;
    if (!findUnsigned(line, "schema", schema) ||
        (schema != 1UL && schema != SchemaVersion) ||
        !findBoolean(line, "enabled", enabled) ||
        !findString(line, "host", host) ||
        !findUnsigned(line, "port", port) || port == 0UL || port > 65535UL ||
        !findString(line, "username", username) ||
        !findString(line, "password_hex", passwordHex) ||
        !decodeHex(passwordHex, password) ||
        !findString(line, "remote_directory", remoteDirectory) ||
        !findUnsigned(line, "retention_count", retention) || retention > 255UL)
    {
        return false;
    }
    settings.enabled = enabled;
    settings.host = host;
    settings.port = static_cast<uint16_t>(port);
    settings.username = username;
    settings.password = password;
    settings.remoteDirectory = remoteDirectory;
    settings.retentionCount = static_cast<uint8_t>(retention);
    if (schema == SchemaVersion)
    {
        uint32_t scheduleHour = 0UL, scheduleMinute = 0UL,
                 lastScheduledDate = 0UL;
        bool scheduleEnabled = false;
        if (!findBoolean(line, "schedule_enabled", scheduleEnabled) ||
            !findUnsigned(line, "schedule_hour", scheduleHour) ||
            scheduleHour > 255UL ||
            !findUnsigned(line, "schedule_minute", scheduleMinute) ||
            scheduleMinute > 255UL ||
            !findUnsigned(line, "last_scheduled_date", lastScheduledDate))
            return false;
        settings.scheduleEnabled = scheduleEnabled;
        settings.scheduleHour = static_cast<uint8_t>(scheduleHour);
        settings.scheduleMinute = static_cast<uint8_t>(scheduleMinute);
        settings.lastScheduledDate = lastScheduledDate;
    }
    return valid(settings);
}
}
