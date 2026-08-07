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

struct NewMotor
{
    String name;
    String model;
    String manufacturer;
    String tags;
    String coilProgram;
    String comment;
};

struct NewRepair
{
    uint32_t clientId;
    uint32_t motorId;
    String receivedAt;
    String complaint;
    String comment;

    NewRepair() : clientId(0UL), motorId(0UL) {}
};

class RepairRegistry
{
public:
    explicit RepairRegistry(fs::FS& storage);

    bool begin();
    bool ready() const;

    bool addClient(const NewClient& client, uint32_t& clientId);
    bool addMotor(const NewMotor& motor, uint32_t& motorId);
    bool addRepair(const NewRepair& repair, uint32_t& repairId);
    bool closeRepair(uint32_t repairId, const String& closedAt,
                     bool& alreadyClosed);
    bool repairIsOpen(uint32_t repairId, bool& open) const;

    bool appendClientsJson(String& json, const String& phoneQuery,
                           uint16_t& count) const;
    bool appendMotorsJson(String& json, const String& query,
                          uint16_t& count) const;
    bool appendSimilarMotorsJson(String& json,
                                 const NewMotor& candidate,
                                 uint16_t& sameProgramCount,
                                 uint16_t& identityMatchCount) const;
    bool appendRepairsJson(String& json, uint32_t clientId,
                           uint16_t& count) const;

    bool clientExists(uint32_t clientId) const;
    bool motorExists(uint32_t motorId) const;

    static String normalizePhone(const String& phone);

private:
    static constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
    static constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    static constexpr const char* RepairStatusPath = "/data/workshop/repair-status.ndjson";

    bool ensureDirectories();
    bool validateUniqueIds(const char* path, const char* key) const;
    bool validateRepairStatusHistory() const;
    bool repairClosed(uint32_t repairId, bool& closed, String& closedAt) const;
    bool nextId(const char* path, const char* key, uint32_t& id) const;
    bool idExists(const char* path, const char* key, uint32_t id) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
