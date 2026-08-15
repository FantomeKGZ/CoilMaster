#ifndef CM_NETWORK_PROFILE_STORE_H
#define CM_NETWORK_PROFILE_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NetworkProfile
{
    uint8_t id = 0U;
    String ssid;
    String password;
    uint8_t priority = 1U;
    bool enabled = true;
    bool hidden = false;
    bool useStaticIp = false;
    String localIp;
    String gateway;
    String subnet;
    String dns1;
    String dns2;
};

class NetworkProfileStore
{
public:
    static constexpr uint8_t MaxProfiles = 5U;

    explicit NetworkProfileStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool load(NetworkProfile* profiles, uint8_t& count) const;
    bool upsert(NetworkProfile& profile);
    bool remove(uint8_t id);

    static bool valid(const NetworkProfile& profile);

private:
    static constexpr const char* Directory = "/data/settings";
    static constexpr const char* ProfilesPath =
        "/data/settings/wifi-profiles.ndjson";
    static constexpr const char* TempPath =
        "/data/settings/wifi-profiles.tmp";
    static constexpr const char* BackupPath =
        "/data/settings/wifi-profiles.bak";

    bool recoverFileSwap();
    bool loadFromPath(const char* path,
                      NetworkProfile* profiles,
                      uint8_t& count) const;
    bool saveAll(const NetworkProfile* profiles, uint8_t count);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_NETWORK_PROFILE_STORE_H
