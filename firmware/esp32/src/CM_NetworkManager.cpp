#include "CM_NetworkManager.h"

namespace CM
{
NetworkManager::NetworkManager(NetworkProfileStore& store)
    : m_store(store), m_count(0U), m_nextIndex(0U), m_activeProfileId(0U),
      m_attemptStartedMs(0UL), m_retryAtMs(0UL), m_ready(false),
      m_connecting(false), m_lastResult("NOT_STARTED") {}

bool NetworkManager::begin(const char* apName, const char* apPassword)
{
    m_ready = false;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(apName, apPassword))
    {
        m_lastResult = "AP_START_FAILED";
        return false;
    }
    if (!loadProfiles())
    {
        m_lastResult = "PROFILE_STORAGE_INVALID";
        return false;
    }
    m_ready = true;
    startNextProfile(millis());
    return true;
}

void NetworkManager::update(uint32_t nowMs)
{
    if (!m_ready) return;
    if (WiFi.status() == WL_CONNECTED)
    {
        if (m_connecting) m_lastResult = "CONNECTED";
        m_connecting = false;
        return;
    }
    if (m_connecting)
    {
        if (nowMs - m_attemptStartedMs < ConnectionTimeoutMs) return;
        WiFi.disconnect(false, false);
        m_connecting = false;
        m_lastResult = "PROFILE_TIMEOUT";
        if (startNextProfile(nowMs)) return;
        m_retryAtMs = nowMs + RetryDelayMs;
        return;
    }
    if (m_count > 0U && static_cast<int32_t>(nowMs - m_retryAtMs) >= 0)
    {
        m_nextIndex = 0U;
        startNextProfile(nowMs);
    }
}

void NetworkManager::reload()
{
    WiFi.disconnect(false, false);
    m_connecting = false;
    m_activeProfileId = 0U;
    if (!loadProfiles())
    {
        m_ready = false;
        m_lastResult = "PROFILE_STORAGE_INVALID";
        return;
    }
    m_ready = true;
    startNextProfile(millis());
}

bool NetworkManager::ready() const { return m_ready; }
bool NetworkManager::connecting() const { return m_connecting; }
uint8_t NetworkManager::activeProfileId() const { return m_activeProfileId; }

const char* NetworkManager::stateName() const
{
    if (!m_ready) return "AP_RECOVERY";
    if (WiFi.status() == WL_CONNECTED) return "AP_STA_CONNECTED";
    if (m_connecting) return "AP_STA_CONNECTING";
    return m_count == 0U ? "AP_NO_PROFILES" : "AP_STA_RETRY_WAIT";
}

const char* NetworkManager::lastResult() const { return m_lastResult; }

bool NetworkManager::loadProfiles()
{
    m_count = 0U;
    m_nextIndex = 0U;
    if (!m_store.load(m_profiles, m_count)) return false;
    uint8_t write = 0U;
    for (uint8_t i = 0U; i < m_count; ++i)
        if (m_profiles[i].enabled) m_profiles[write++] = m_profiles[i];
    m_count = write;
    if (m_count == 0U) m_lastResult = "NO_ENABLED_PROFILES";
    return true;
}

bool NetworkManager::startNextProfile(uint32_t nowMs)
{
    if (m_nextIndex >= m_count)
    {
        m_activeProfileId = 0U;
        m_connecting = false;
        m_retryAtMs = nowMs + RetryDelayMs;
        return false;
    }
    const NetworkProfile& profile = m_profiles[m_nextIndex++];
    m_activeProfileId = profile.id;
    m_attemptStartedMs = nowMs;
    m_connecting = true;
    m_lastResult = "CONNECTING";
    bool configured = false;
    if (profile.useStaticIp)
    {
        IPAddress local, gateway, subnet, dns1, dns2;
        if (local.fromString(profile.localIp) && gateway.fromString(profile.gateway) &&
            subnet.fromString(profile.subnet) &&
            (profile.dns1.length() == 0U || dns1.fromString(profile.dns1)) &&
            (profile.dns2.length() == 0U || dns2.fromString(profile.dns2)))
        {
            if (profile.dns1.length() == 0U) dns1 = gateway;
            configured = WiFi.config(local, gateway, subnet, dns1, dns2);
        }
    }
    else
    {
        configured = WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }
    if (!configured)
    {
        m_connecting = false;
        m_lastResult = "IP_CONFIG_FAILED";
        return startNextProfile(nowMs);
    }
    WiFi.begin(profile.ssid.c_str(), profile.password.c_str());
    return true;
}
}
