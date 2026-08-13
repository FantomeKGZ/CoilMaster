#include "CM_NetworkProfileStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
constexpr uint32_t SchemaVersion = 1UL;

char hexDigit(uint8_t value)
{
    return value < 10U ? static_cast<char>('0' + value)
                       : static_cast<char>('A' + value - 10U);
}

bool fromHex(char source, uint8_t& value)
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
    String result;
    result.reserve(source.length() * 2U);
    for (size_t i = 0U; i < source.length(); ++i)
    {
        const uint8_t value = static_cast<uint8_t>(source[i]);
        result += hexDigit(static_cast<uint8_t>(value >> 4U));
        result += hexDigit(static_cast<uint8_t>(value & 0x0FU));
    }
    return result;
}

bool decodeHex(const String& source, String& value, size_t maximumBytes)
{
    value = String();
    if ((source.length() % 2U) != 0U || source.length() > maximumBytes * 2U)
        return false;
    value.reserve(source.length() / 2U);
    for (size_t i = 0U; i < source.length(); i += 2U)
    {
        uint8_t high = 0U, low = 0U;
        if (!fromHex(source[i], high) || !fromHex(source[i + 1U], low))
            return false;
        value += static_cast<char>((high << 4U) | low);
    }
    return true;
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
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}')) return false;
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
    else return false;
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
        return false;
    value = line.substring(valueStart, valueEnd);
    return true;
}

void sortProfiles(NetworkProfile* profiles, uint8_t count)
{
    for (uint8_t i = 1U; i < count; ++i)
    {
        NetworkProfile selected = profiles[i];
        uint8_t position = i;
        while (position > 0U &&
               (profiles[position - 1U].priority > selected.priority ||
                (profiles[position - 1U].priority == selected.priority &&
                 profiles[position - 1U].id > selected.id)))
        {
            profiles[position] = profiles[position - 1U];
            --position;
        }
        profiles[position] = selected;
    }
}
}

NetworkProfileStore::NetworkProfileStore(fs::FS& storage)
    : m_storage(storage), m_ready(false) {}

bool NetworkProfileStore::begin()
{
    m_ready = false;
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists(Directory) && !m_storage.mkdir(Directory)) return false;
    if (!recoverFileSwap()) return false;
    if (m_storage.exists(ProfilesPath))
    {
        NetworkProfile profiles[MaxProfiles];
        uint8_t count = 0U;
        if (!loadFromPath(ProfilesPath, profiles, count)) return false;
    }
    m_ready = true;
    return true;
}

bool NetworkProfileStore::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open(Directory, FILE_READ);
    if (!directory) return false;
    const bool available = directory.isDirectory();
    directory.close();
    return available;
}

bool NetworkProfileStore::load(NetworkProfile* profiles, uint8_t& count) const
{
    count = 0U;
    if (!ready() || profiles == nullptr) return false;
    if (!m_storage.exists(ProfilesPath)) return true;
    return loadFromPath(ProfilesPath, profiles, count);
}

bool NetworkProfileStore::upsert(NetworkProfile& profile)
{
    if (!ready() || !valid(profile)) return false;
    NetworkProfile profiles[MaxProfiles];
    uint8_t count = 0U;
    if (!load(profiles, count)) return false;

    int target = -1;
    if (profile.id != 0U)
    {
        for (uint8_t i = 0U; i < count; ++i)
            if (profiles[i].id == profile.id) target = i;
        if (target < 0) return false;
    }
    else
    {
        if (count >= MaxProfiles) return false;
        for (uint8_t candidate = 1U; candidate <= MaxProfiles; ++candidate)
        {
            bool used = false;
            for (uint8_t i = 0U; i < count; ++i)
                if (profiles[i].id == candidate) used = true;
            if (!used)
            {
                profile.id = candidate;
                break;
            }
        }
        target = count;
        ++count;
    }
    profiles[target] = profile;
    sortProfiles(profiles, count);
    return saveAll(profiles, count);
}

bool NetworkProfileStore::remove(uint8_t id)
{
    if (!ready() || id == 0U || id > MaxProfiles) return false;
    NetworkProfile profiles[MaxProfiles];
    uint8_t count = 0U;
    if (!load(profiles, count)) return false;
    uint8_t write = 0U;
    bool found = false;
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (profiles[i].id == id) found = true;
        else profiles[write++] = profiles[i];
    }
    return found && saveAll(profiles, write);
}

bool NetworkProfileStore::valid(const NetworkProfile& profile)
{
    if (profile.id > MaxProfiles || profile.priority < 1U ||
        profile.priority > MaxProfiles || profile.ssid.length() == 0U ||
        profile.ssid.length() > 32U || profile.password.length() > 63U ||
        (profile.password.length() > 0U && profile.password.length() < 8U))
        return false;
    for (size_t i = 0U; i < profile.ssid.length(); ++i)
        if (static_cast<uint8_t>(profile.ssid[i]) < 0x20U) return false;
    for (size_t i = 0U; i < profile.password.length(); ++i)
    {
        const uint8_t value = static_cast<uint8_t>(profile.password[i]);
        if (value < 0x20U || value > 0x7EU) return false;
    }
    return true;
}

bool NetworkProfileStore::recoverFileSwap()
{
    const bool mainExists = m_storage.exists(ProfilesPath);
    const bool tempExists = m_storage.exists(TempPath);
    const bool backupExists = m_storage.exists(BackupPath);
    if (!tempExists && !backupExists) return true;
    NetworkProfile profiles[MaxProfiles];
    uint8_t count = 0U;
    if (mainExists && loadFromPath(ProfilesPath, profiles, count))
    {
        if (tempExists && !m_storage.remove(TempPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }
    uint8_t tempCount = 0U, backupCount = 0U;
    NetworkProfile tempProfiles[MaxProfiles], backupProfiles[MaxProfiles];
    const bool tempValid = tempExists &&
        loadFromPath(TempPath, tempProfiles, tempCount);
    const bool backupValid = backupExists &&
        loadFromPath(BackupPath, backupProfiles, backupCount);
    if (mainExists && !m_storage.remove(ProfilesPath)) return false;
    if (tempValid)
    {
        if (!m_storage.rename(TempPath, ProfilesPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }
    if (tempExists && !m_storage.remove(TempPath)) return false;
    if (backupValid) return m_storage.rename(BackupPath, ProfilesPath);
    return false;
}

bool NetworkProfileStore::loadFromPath(const char* path,
                                        NetworkProfile* profiles,
                                        uint8_t& count) const
{
    count = 0U;
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (count >= MaxProfiles || !FlatJsonObjectValidator::valid(line))
        {
            file.close();
            count = 0U;
            return false;
        }
        uint32_t schema = 0UL, id = 0UL, priority = 0UL;
        bool enabled = false, hidden = false;
        String ssidHex, passwordHex;
        NetworkProfile profile;
        if (!findUnsigned(line, "schema", schema) || schema != SchemaVersion ||
            !findUnsigned(line, "id", id) || id == 0UL || id > MaxProfiles ||
            !findString(line, "ssid_hex", ssidHex) ||
            !decodeHex(ssidHex, profile.ssid, 32U) ||
            !findString(line, "password_hex", passwordHex) ||
            !decodeHex(passwordHex, profile.password, 63U) ||
            !findUnsigned(line, "priority", priority) ||
            priority == 0UL || priority > MaxProfiles ||
            !findBoolean(line, "enabled", enabled) ||
            !findBoolean(line, "hidden", hidden))
        {
            file.close(); count = 0U; return false;
        }
        profile.id = static_cast<uint8_t>(id);
        profile.priority = static_cast<uint8_t>(priority);
        profile.enabled = enabled;
        profile.hidden = hidden;
        if (!valid(profile)) { file.close(); count = 0U; return false; }
        for (uint8_t i = 0U; i < count; ++i)
            if (profiles[i].id == profile.id) { file.close(); count = 0U; return false; }
        profiles[count++] = profile;
    }
    file.close();
    sortProfiles(profiles, count);
    return true;
}

bool NetworkProfileStore::saveAll(const NetworkProfile* profiles, uint8_t count)
{
    if (!ready() || profiles == nullptr || count > MaxProfiles) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file) return false;
    bool written = true;
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (!valid(profiles[i])) { written = false; break; }
        String line = F("{\"schema\":1,\"id\":"); line += profiles[i].id;
        line += F(",\"ssid_hex\":\""); line += encodeHex(profiles[i].ssid);
        line += F("\",\"password_hex\":\""); line += encodeHex(profiles[i].password);
        line += F("\",\"priority\":"); line += profiles[i].priority;
        line += F(",\"enabled\":"); line += profiles[i].enabled ? F("true") : F("false");
        line += F(",\"hidden\":"); line += profiles[i].hidden ? F("true") : F("false");
        line += F("}\n");
        if (file.print(line) != line.length()) { written = false; break; }
    }
    file.flush(); file.close();
    NetworkProfile verified[MaxProfiles];
    uint8_t verifiedCount = 0U;
    if (!written || !loadFromPath(TempPath, verified, verifiedCount) ||
        verifiedCount != count)
    {
        m_storage.remove(TempPath); return false;
    }
    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    { m_storage.remove(TempPath); return false; }
    if (m_storage.exists(ProfilesPath) && !m_storage.rename(ProfilesPath, BackupPath))
    { m_storage.remove(TempPath); return false; }
    if (!m_storage.rename(TempPath, ProfilesPath))
    {
        if (!m_storage.exists(ProfilesPath) && m_storage.exists(BackupPath))
            m_storage.rename(BackupPath, ProfilesPath);
        return false;
    }
    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    { m_ready = false; return false; }
    return true;
}
}
