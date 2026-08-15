#ifndef CM_NETWORK_MANAGER_H
#define CM_NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

#include "CM_NetworkProfileStore.h"

namespace CM
{
class NetworkManager
{
public:
    explicit NetworkManager(NetworkProfileStore& store);

    bool begin(const char* apName, const char* apPassword);
    void update(uint32_t nowMs);
    void reload();
    bool prepareScan();

    bool ready() const;
    bool connecting() const;
    uint8_t activeProfileId() const;
    const char* stateName() const;
    const char* lastResult() const;

private:
    static constexpr uint32_t ConnectionTimeoutMs = 15000UL;
    static constexpr uint32_t RetryDelayMs = 30000UL;
    static constexpr uint32_t ScanGraceMs = 15000UL;

    bool loadProfiles();
    bool startNextProfile(uint32_t nowMs);

    NetworkProfileStore& m_store;
    NetworkProfile m_profiles[NetworkProfileStore::MaxProfiles];
    uint8_t m_count;
    uint8_t m_nextIndex;
    uint8_t m_activeProfileId;
    uint32_t m_attemptStartedMs;
    uint32_t m_retryAtMs;
    bool m_ready;
    bool m_connecting;
    const char* m_lastResult;
};
}

#endif // CM_NETWORK_MANAGER_H
