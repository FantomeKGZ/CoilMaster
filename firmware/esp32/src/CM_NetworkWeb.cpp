#include "CM_NetworkWeb.h"

namespace CM
{
namespace
{
bool parseUnsigned(const String& source, uint32_t maximum, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0')) return false;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (value > (0xFFFFFFFFUL - digit) / 10UL) return false;
        value = value * 10UL + digit;
    }
    return value <= maximum;
}

bool parseBoolean(const String& source, bool& value)
{
    if (source == "1" || source == "true") { value = true; return true; }
    if (source == "0" || source == "false") { value = false; return true; }
    return false;
}

String escaped(const String& value)
{
    static const char Hex[] = "0123456789abcdef";
    String result;
    result.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const uint8_t byte = static_cast<uint8_t>(value[i]);
        switch (byte)
        {
            case '"': result += F("\\\""); break;
            case '\\': result += F("\\\\"); break;
            case '\b': result += F("\\b"); break;
            case '\f': result += F("\\f"); break;
            case '\n': result += F("\\n"); break;
            case '\r': result += F("\\r"); break;
            case '\t': result += F("\\t"); break;
            default:
                if (byte < 0x20U)
                {
                    result += F("\\u00");
                    result += Hex[(byte >> 4U) & 0x0FU];
                    result += Hex[byte & 0x0FU];
                }
                else
                {
                    result += static_cast<char>(byte);
                }
                break;
        }
    }
    return result;
}
}

NetworkWeb::NetworkWeb(WebServer& server,
                       NetworkProfileStore& store,
                       NetworkManager& manager)
    : m_server(server), m_store(store), m_manager(manager),
      m_scanRequested(false) {}

void NetworkWeb::begin()
{
    m_server.on("/api/network/profiles", HTTP_GET,
                [this]() { handleProfiles(); });
    m_server.on("/api/network/profiles", HTTP_POST,
                [this]() { handleSave(); });
    m_server.on("/api/network/profiles/delete", HTTP_POST,
                [this]() { handleDelete(); });
    m_server.on("/api/network/reconnect", HTTP_POST,
                [this]() { handleReconnect(); });
    m_server.on("/api/network/scan", HTTP_POST,
                [this]() { handleScanStart(); });
    m_server.on("/api/network/scan", HTTP_GET,
                [this]() { handleScanResult(); });
}

void NetworkWeb::handleProfiles()
{
    NetworkProfile profiles[NetworkProfileStore::MaxProfiles];
    uint8_t count = 0U;
    if (!m_store.load(profiles, count))
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }
    String response = F("{\"items\":[");
    response.reserve(180U + static_cast<unsigned int>(count) * 150U);
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (i > 0U) response += ',';
        response += F("{\"id\":"); response += profiles[i].id;
        response += F(",\"ssid\":\""); response += escaped(profiles[i].ssid);
        response += F("\",\"priority\":"); response += profiles[i].priority;
        response += F(",\"enabled\":"); response += profiles[i].enabled ? F("true") : F("false");
        response += F(",\"hidden\":"); response += profiles[i].hidden ? F("true") : F("false");
        response += F(",\"use_static_ip\":"); response += profiles[i].useStaticIp ? F("true") : F("false");
        response += F(",\"local_ip\":\""); response += profiles[i].localIp;
        response += F("\",\"gateway\":\""); response += profiles[i].gateway;
        response += F("\",\"subnet\":\""); response += profiles[i].subnet;
        response += F("\",\"dns1\":\""); response += profiles[i].dns1;
        response += F("\",\"dns2\":\""); response += profiles[i].dns2;
        response += '"';
        response += F(",\"password_configured\":"); response += profiles[i].password.length() > 0U ? F("true") : F("false");
        response += '}';
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"max_profiles\":"); response += NetworkProfileStore::MaxProfiles;
    response += F(",\"credentials_exposed\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void NetworkWeb::handleSave()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("ssid") || !m_server.hasArg("priority") ||
        !m_server.hasArg("enabled") || !m_server.hasArg("hidden"))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"network_profile_fields_required\"}");
        return;
    }

    uint32_t id = 0UL, priority = 0UL;
    bool enabled = false, hidden = false, useStaticIp = false;
    if ((m_server.hasArg("id") && m_server.arg("id").length() > 0U &&
         !parseUnsigned(m_server.arg("id"), NetworkProfileStore::MaxProfiles, id)) ||
        !parseUnsigned(m_server.arg("priority"), NetworkProfileStore::MaxProfiles, priority) ||
        priority == 0UL || !parseBoolean(m_server.arg("enabled"), enabled) ||
        !parseBoolean(m_server.arg("hidden"), hidden) ||
        (m_server.hasArg("use_static_ip") &&
         !parseBoolean(m_server.arg("use_static_ip"), useStaticIp)))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"invalid_network_profile_fields\"}");
        return;
    }

    NetworkProfile profile;
    profile.id = static_cast<uint8_t>(id);
    profile.ssid = m_server.arg("ssid");
    profile.priority = static_cast<uint8_t>(priority);
    profile.enabled = enabled;
    profile.hidden = hidden;
    profile.useStaticIp = useStaticIp;
    if (useStaticIp)
    {
        if (!m_server.hasArg("local_ip") || !m_server.hasArg("gateway") ||
            !m_server.hasArg("subnet"))
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"static_ip_fields_required\"}");
            return;
        }
        profile.localIp = m_server.arg("local_ip");
        profile.gateway = m_server.arg("gateway");
        profile.subnet = m_server.arg("subnet");
        if (m_server.hasArg("dns1")) profile.dns1 = m_server.arg("dns1");
        if (m_server.hasArg("dns2")) profile.dns2 = m_server.arg("dns2");
    }

    NetworkProfile existing[NetworkProfileStore::MaxProfiles];
    uint8_t count = 0U;
    if (!m_store.load(existing, count))
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }
    if (profile.id != 0U)
    {
        bool found = false;
        for (uint8_t i = 0U; i < count; ++i)
        {
            if (existing[i].id != profile.id) continue;
            profile.password = existing[i].password;
            found = true;
            break;
        }
        if (!found)
        {
            m_server.send(404, "application/json",
                          "{\"error\":\"network_profile_not_found\"}");
            return;
        }
    }
    else if (count >= NetworkProfileStore::MaxProfiles)
    {
        m_server.send(409, "application/json",
                      "{\"error\":\"network_profile_capacity_reached\"}");
        return;
    }

    if (m_server.hasArg("password") && m_server.arg("password").length() > 0U)
        profile.password = m_server.arg("password");
    if (!NetworkProfileStore::valid(profile))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"invalid_network_profile\"}");
        return;
    }
    if (!m_store.upsert(profile))
    {
        m_server.send(m_store.ready() ? 500 : 503,
                      "application/json",
                      m_store.ready()
                          ? "{\"error\":\"network_profile_persistence_failed\"}"
                          : "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }

    m_manager.reload();
    if (!m_manager.ready())
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_manager_reload_failed\",\"saved\":true}");
        return;
    }
    String response = F("{\"saved\":true,\"id\":"); response += profile.id;
    response += F(",\"credentials_exposed\":false}");
    m_server.send(200, "application/json", response);
}

void NetworkWeb::handleDelete()
{
    if (!m_store.ready())
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }

    uint32_t id = 0UL;
    if (!m_server.hasArg("id") ||
        !parseUnsigned(m_server.arg("id"), NetworkProfileStore::MaxProfiles, id) ||
        id == 0UL)
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"invalid_network_profile_id\"}");
        return;
    }

    NetworkProfile existing[NetworkProfileStore::MaxProfiles];
    uint8_t count = 0U;
    if (!m_store.load(existing, count))
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }
    bool found = false;
    for (uint8_t i = 0U; i < count; ++i)
        if (existing[i].id == static_cast<uint8_t>(id)) { found = true; break; }
    if (!found)
    {
        m_server.send(404, "application/json",
                      "{\"error\":\"network_profile_not_found\"}");
        return;
    }

    if (!m_store.remove(static_cast<uint8_t>(id)))
    {
        m_server.send(m_store.ready() ? 500 : 503,
                      "application/json",
                      m_store.ready()
                          ? "{\"error\":\"network_profile_delete_persistence_failed\"}"
                          : "{\"error\":\"network_profiles_unavailable\"}");
        return;
    }
    m_manager.reload();
    if (!m_manager.ready())
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_manager_reload_failed\",\"deleted\":true}");
        return;
    }
    m_server.send(200, "application/json", "{\"deleted\":true}");
}

void NetworkWeb::handleReconnect()
{
    m_manager.reload();
    if (!m_manager.ready())
    {
        m_server.send(503, "application/json",
                      "{\"error\":\"network_manager_reload_failed\"}");
        return;
    }
    m_server.send(202, "application/json", "{\"reconnecting\":true}");
}

void NetworkWeb::handleScanStart()
{
    const int16_t current = WiFi.scanComplete();
    if (m_scanRequested && current == WIFI_SCAN_RUNNING)
    {
        m_server.send(202, "application/json",
                      "{\"started\":false,\"active\":true}");
        return;
    }
    if (!m_manager.prepareScan())
    {
        m_server.send(409, "application/json",
                      "{\"error\":\"network_connection_in_progress\"}");
        return;
    }
    WiFi.scanDelete();
    const int16_t started = WiFi.scanNetworks(true, true);
    if (started != WIFI_SCAN_RUNNING)
    {
        m_scanRequested = false;
        m_server.send(500, "application/json",
                      "{\"error\":\"network_scan_start_failed\"}");
        return;
    }
    m_scanRequested = true;
    m_server.send(202, "application/json",
                  "{\"started\":true,\"active\":true}");
}

void NetworkWeb::handleScanResult()
{
    if (!m_scanRequested)
    {
        m_server.send(200, "application/json",
                      "{\"active\":false,\"complete\":false,\"items\":[],\"count\":0,\"max_results\":20}");
        return;
    }
    const int16_t found = WiFi.scanComplete();
    if (found == WIFI_SCAN_RUNNING)
    {
        m_server.send(200, "application/json",
                      "{\"active\":true,\"complete\":false,\"items\":[],\"count\":0,\"max_results\":20}");
        return;
    }
    if (found < 0)
    {
        m_scanRequested = false;
        WiFi.scanDelete();
        m_server.send(500, "application/json",
                      "{\"error\":\"network_scan_failed\"}");
        return;
    }

    constexpr uint8_t MaxResults = 20U;
    int16_t selected[MaxResults];
    uint8_t count = 0U;
    for (int16_t i = 0; i < found && count < MaxResults; ++i)
    {
        const String ssid = WiFi.SSID(i);
        if (ssid.length() == 0U || ssid.length() > 32U) continue;
        bool duplicate = false;
        for (uint8_t j = 0U; j < count; ++j)
        {
            if (WiFi.SSID(selected[j]) == ssid) { duplicate = true; break; }
        }
        if (!duplicate) selected[count++] = i;
    }

    String response = F("{\"active\":false,\"complete\":true,\"items\":[");
    response.reserve(160U + static_cast<unsigned int>(count) * 100U);
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (i > 0U) response += ',';
        const int16_t index = selected[i];
        response += F("{\"ssid\":\""); response += escaped(WiFi.SSID(index));
        response += F("\",\"rssi\":"); response += WiFi.RSSI(index);
        response += F(",\"encrypted\":");
        response += WiFi.encryptionType(index) == WIFI_AUTH_OPEN ? F("false") : F("true");
        response += F(",\"channel\":"); response += WiFi.channel(index);
        response += '}';
    }
    response += F("],\"count\":"); response += count;
    response += F(",\"total_found\":"); response += found;
    response += F(",\"max_results\":20,\"duplicates_removed\":true}");
    m_scanRequested = false;
    m_server.send(200, "application/json; charset=utf-8", response);
    WiFi.scanDelete();
}
}
