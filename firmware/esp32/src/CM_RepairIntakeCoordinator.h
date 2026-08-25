#ifndef CM_REPAIR_INTAKE_COORDINATOR_H
#define CM_REPAIR_INTAKE_COORDINATOR_H

#include <Arduino.h>
#include <FS.h>

#include "CM_MotorWindingVersionStore.h"
#include "CM_RepairAsReceivedSnapshotStore.h"
#include "CM_RepairIntakePendingStore.h"
#include "CM_RepairRegistry.h"

namespace CM
{
enum class RepairIntakeCreateResult : uint8_t
{
    Created = 0U,
    Unavailable,
    Busy,
    InvalidSource,
    RegistryRejected,
    IntegrityFailed
};

class RepairIntakeCoordinator
{
public:
    RepairIntakeCoordinator(fs::FS& storage, RepairRegistry& registry);

    bool begin();
    bool ready() const;
    RepairIntakeCreateResult create(const NewRepair& repair, uint32_t& repairId);

private:
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";

    bool recoverPending();
    bool nextExpectedRepairId(uint32_t& repairId) const;
    bool preparePending(const NewRepair& repair,
                        uint32_t repairId,
                        RepairIntakePending& pending);
    bool buildSnapshot(const RepairIntakePending& pending,
                       NewRepairAsReceivedSnapshot& snapshot) const;
    bool repairMatchesPending(uint32_t repairId,
                              const RepairIntakePending& pending,
                              bool& found) const;
    bool snapshotMatchesPending(uint32_t repairId,
                                const RepairIntakePending& pending,
                                bool& found) const;

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findOptionalUnsigned(const String& line,
                                     const char* key,
                                     uint32_t& value,
                                     bool& present);
    static bool findBoolean(const String& line, const char* key, bool& value);
    static bool findString(const String& line, const char* key, String& value);
    static bool findOptionalString(const String& line,
                                   const char* key,
                                   String& value,
                                   bool& present);

    fs::FS& m_storage;
    RepairRegistry& m_registry;
    MotorWindingVersionStore m_windingVersions;
    RepairAsReceivedSnapshotStore m_snapshots;
    RepairIntakePendingStore m_pending;
    bool m_ready;
};
}

#endif
