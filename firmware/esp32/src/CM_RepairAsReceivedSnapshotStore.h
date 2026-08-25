#ifndef CM_REPAIR_AS_RECEIVED_SNAPSHOT_STORE_H
#define CM_REPAIR_AS_RECEIVED_SNAPSHOT_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NewRepairAsReceivedSnapshot
{
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;
    uint32_t windingVersionId;
    String capturedAt;
    String motorName;
    String manufacturer;
    String model;
    uint8_t phases;
    uint16_t slotCount;
    String workingProgram;
    uint16_t workingRepeatTarget;
    String workingConductors;
    bool startingPresent;
    String startingProgram;
    uint16_t startingRepeatTarget;
    String startingConductors;
    String sourceKind;
    String comment;

    NewRepairAsReceivedSnapshot()
        : repairId(0UL), clientId(0UL), motorId(0UL), windingVersionId(0UL),
          phases(0U), slotCount(0U), workingRepeatTarget(1U),
          startingPresent(false), startingRepeatTarget(1U)
    {
    }
};

class RepairAsReceivedSnapshotStore
{
public:
    static constexpr const char* Path = "/data/workshop/repair-as-received.ndjson";

    explicit RepairAsReceivedSnapshotStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewRepairAsReceivedSnapshot& snapshot, uint32_t& snapshotId);
    bool appendByRepairIdJson(String& json, uint32_t repairId, bool& found) const;

private:
    bool ensureDirectory();
    bool nextSnapshotId(uint32_t& snapshotId) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
