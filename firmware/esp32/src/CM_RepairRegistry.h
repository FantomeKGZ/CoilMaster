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
    uint32_t ratedPowerW;
    uint16_t ratedVoltageV;
    uint32_t ratedCurrentMa;
    uint16_t ratedSpeedRpm;
    uint16_t frequencyHz;
    uint8_t phases;
    uint16_t slotCount;
    uint16_t repeatTarget;
    uint8_t poleCount;
    uint16_t coilPitch;
    uint16_t turnsPerCoil;
    uint16_t wireDiameterHundredthsMm;
    uint8_t parallelStrands;
    uint16_t statorBoreMm;
    uint16_t statorCoreLengthMm;
    String connection;
    String windingType;
    String wireMaterial;
    String sourceType;
    String sourceUrl;
    String sourceTitle;
    String sourceRetrievedAt;
    String confidence;
    bool calculatedFields;

    NewMotor()
        : ratedPowerW(0UL), ratedVoltageV(0U), ratedCurrentMa(0UL),
          ratedSpeedRpm(0U), frequencyHz(0U), phases(0U), slotCount(0U),
          repeatTarget(1U), poleCount(0U), coilPitch(0U), turnsPerCoil(0U),
          wireDiameterHundredthsMm(0U), parallelStrands(0U),
          statorBoreMm(0U), statorCoreLengthMm(0U),
          calculatedFields(false)
    {
    }
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

struct RepairIdentity
{
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;

    RepairIdentity() : repairId(0UL), clientId(0UL), motorId(0UL) {}
};

class RepairRegistry
{
public:
    static constexpr uint8_t MaxListPageSize = 32U;

    explicit RepairRegistry(fs::FS& storage);

    bool begin();
    bool ready() const;

    bool addClient(const NewClient& client, uint32_t& clientId);
    bool updateClient(uint32_t clientId, const NewClient& client);
    bool addMotor(const NewMotor& motor, uint32_t& motorId);
    bool updateMotor(uint32_t motorId, const NewMotor& motor);
    bool addRepair(const NewRepair& repair, uint32_t& repairId);
    bool closeRepair(uint32_t repairId, const String& closedAt,
                     bool& alreadyClosed);
    bool repairIsOpen(uint32_t repairId, bool& open) const
    {
        open = false;
        bool repairFound = false;
        if (!ready() || repairId == 0UL ||
            !idExists(RepairsPath, "repair_id", repairId, repairFound) ||
            !repairFound)
        {
            return false;
        }
        return repairStatusIsOpen(repairId, open);
    }

    // Caller must already have validated the repair through an authoritative
    // repairs.ndjson lookup in the same operation. This reads only status state.
    bool repairStatusIsOpen(uint32_t repairId, bool& open) const
    {
        open = false;
        bool closed = false;
        String closedAt;
        if (!repairClosed(repairId, closed, closedAt)) return false;
        open = !closed;
        return true;
    }

    bool loadRepairIdentity(uint32_t repairId,
                            RepairIdentity& identity,
                            bool& found) const;

    bool appendSimilarMotorsJson(String& json,
                                 const NewMotor& candidate,
                                 uint16_t& sameProgramCount,
                                 uint16_t& identityMatchCount,
                                 uint8_t& returnedCount,
                                 bool& itemsTruncated) const;

    bool appendClientsPageJson(String& json,
                               const String& phoneQuery,
                               uint32_t cursor,
                               uint8_t limit,
                               uint16_t& count,
                               uint32_t& nextCursor,
                               bool& hasMore) const;
    bool appendMotorsPageJson(String& json,
                              const String& query,
                              uint32_t cursor,
                              uint8_t limit,
                              uint16_t& count,
                              uint32_t& nextCursor,
                              bool& hasMore) const;

    bool appendClientsSearchPageJson(String& json,
                                     const String& query,
                                     uint32_t cursor,
                                     uint8_t limit,
                                     uint16_t& count,
                                     uint32_t& nextCursor,
                                     bool& hasMore) const;
    bool appendRepairsSearchPageJson(String& json,
                                     const String& query,
                                     uint32_t cursor,
                                     uint8_t limit,
                                     uint16_t& count,
                                     uint32_t& nextCursor,
                                     bool& hasMore) const;

    bool appendRepairsPageJson(String& json,
                               uint32_t clientId,
                               const String& statusFilter,
                               uint32_t cursor,
                               uint8_t limit,
                               uint16_t& count,
                               uint32_t& nextCursor,
                               bool& hasMore) const
    {
        return appendRepairsPageJson(json,
                                     clientId,
                                     0UL,
                                     statusFilter,
                                     cursor,
                                     limit,
                                     count,
                                     nextCursor,
                                     hasMore);
    }

    bool appendRepairsPageJson(String& json,
                               uint32_t clientId,
                               uint32_t motorId,
                               const String& statusFilter,
                               uint32_t cursor,
                               uint8_t limit,
                               uint16_t& count,
                               uint32_t& nextCursor,
                               bool& hasMore) const;

    bool appendClientByIdJson(String& json, uint32_t clientId, bool& found) const;
    bool appendMotorByIdJson(String& json, uint32_t motorId, bool& found) const;
    bool appendRepairByIdJson(String& json, uint32_t repairId, bool& found) const;

    bool clientExists(uint32_t clientId, bool& found) const;
    bool motorExists(uint32_t motorId, bool& found) const;

    static String normalizePhone(const String& phone);

private:
    static constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
    static constexpr const char* ClientRevisionsPath = "/data/workshop/client-revisions.ndjson";
    static constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
    static constexpr const char* MotorRevisionsPath = "/data/workshop/motor-revisions.ndjson";
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    static constexpr const char* RepairStatusPath = "/data/workshop/repair-status.ndjson";

    bool ensureDirectories();
    bool repairClosed(uint32_t repairId, bool& closed, String& closedAt) const;
    bool resolveRepairPageStatuses(const uint32_t* repairIds,
                                   uint8_t repairCount,
                                   bool* closed,
                                   String* closedAt) const;
    bool latestClientRevisionLine(uint32_t clientId,
                                  String& line,
                                  bool& found) const;
    bool latestMotorRevisionLine(uint32_t motorId,
                                 String& line,
                                 bool& found) const;
    bool nextId(const char* path, const char* key, uint32_t& id) const;
    bool idExists(const char* path, const char* key, uint32_t id, bool& found) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
