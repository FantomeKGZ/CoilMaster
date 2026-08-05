#ifndef CM_REPAIR_REGISTRY_H
#define CM_REPAIR_REGISTRY_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NewClient
{
    String name;
    String phone;
    String comment;
};

struct NewRepair
{
    uint32_t clientId;
    String motorName;
    String receivedAt;
    String complaint;
    String comment;

    NewRepair() : clientId(0UL) {}
};

class RepairRegistry
{
public:
    explicit RepairRegistry(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool addClient(const NewClient& client, uint32_t& clientId);
    bool addRepair(const NewRepair& repair, uint32_t& repairId);
    bool appendClientsJson(String& json, const String& phoneQuery, uint16_t& count) const;
    bool appendRepairsJson(String& json, uint32_t clientId, uint16_t& count) const;
    bool clientExists(uint32_t clientId) const;

    static String normalizePhone(const String& phone);

private:
    static constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";

    bool ensureDirectories();
    bool nextId(const char* path, const char* key, uint32_t& id) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
