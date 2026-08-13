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
    String result;
    result.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        if (value[i] == '"' || value[i] == '\\') result += '\\';
        result += value[i];
    }
    return result;
}
}

NetworkWeb::NetworkWeb(WebServer& server,
                       NetworkProfileStore& store,
                       NetworkManager& manager)
    : m_server(server), m_store(store), m_manager(manager) {}

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
    if (!m_store.ready() || !m_server.hasArg("ssid") ||
        !m_server.hasArg("priority") || !m_server.hasArg("enabled") ||
        !m_server.hasArg("hidden"))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"network_profile_fields_required\"}");
        return;
    }
    uint32_t id = 0UL, priority = 0UL;
    bool enabled = false, hidden = false;
    if ((m_server.hasArg("id") && m_server.arg("id").length() > 0U &&
         !parseUnsigned(m_server.arg("id"), NetworkProfileStore::MaxProfiles, id)) ||
        !parseUnsigned(m_server.arg("priority"), NetworkProfileStore::MaxProfiles, priority) ||
        priority == 0UL || !parseBoolean(m_server.arg("enabled"), enabled) ||
        !parseBoolean(m_server.arg("hidden"), hidden))
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
    if (profile.id != 0U)
    {
        NetworkProfile existing[NetworkProfileStore::MaxProfiles];
        uint8_t count = 0U;
        if (!m_store.load(existing, count))
        {
            m_server.send(503, "application/json",
                          "{\"error\":\"network_profiles_unavailable\"}");
            return;
        }
        for (uint8_t i = 0U; i < count; ++i)
            if (existing[i].id == profile.id) profile.password = existing[i].password;
    }
    if (m_server.hasArg("password") && m_server.arg("password").length() > 0U)
        profile.password = m_server.arg("password");
    if (!NetworkProfileStore::valid(profile) || !m_store.upsert(profile))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"network_profile_save_failed\"}");
        return;
    }
    m_manager.reload();
    String response = F("{\"saved\":true,\"id\":"); response += profile.id;
    response += F(",\"credentials_exposed\":false}");
    m_server.send(200, "application/json", response);
}

void NetworkWeb::handleDelete()
{
    uint32_t id = 0UL;
    if (!m_server.hasArg("id") ||
        !parseUnsigned(m_server.arg("id"), NetworkProfileStore::MaxProfiles, id) ||
        id == 0UL || !m_store.remove(static_cast<uint8_t>(id)))
    {
        m_server.send(400, "application/json",
                      "{\"error\":\"network_profile_delete_failed\"}");
        return;
    }
    m_manager.reload();
    m_server.send(200, "application/json", "{\"deleted\":true}");
}

void NetworkWeb::handleReconnect()
{
    m_manager.reload();
    m_server.send(202, "application/json", "{\"reconnecting\":true}");
}
}
