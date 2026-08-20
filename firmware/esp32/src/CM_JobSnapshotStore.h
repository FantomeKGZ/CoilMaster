#pragma once

#include <Arduino.h>
#include <FS.h>

#include "CM_UartEventReceiver.h"

namespace CM
{
struct JobLinkage
{
    bool linked;
    uint32_t repairId;
    uint32_t motorId;

    JobLinkage();
    static JobLinkage unlinked();
    static JobLinkage linkedTo(uint32_t repairId, uint32_t motorId);
    bool isValid() const;
};

struct JobSnapshot
{
    static constexpr uint8_t MaxCoils = 10U;

    uint32_t jobId;
    uint32_t sessionId;
    JobLinkage linkage;
    RemoteJobType type;
    uint16_t repeatTarget;
    uint8_t coilCount;
    uint16_t turns[MaxCoils];
    uint32_t createdUptimeMs;

    JobSnapshot();
};

class JobSnapshotStore
{
public:
    explicit JobSnapshotStore(fs::FS& fileSystem);

    bool begin();
    bool isReady() const;

    // Compatibility path for jobs that are not yet connected to a repair.
    // Persists repair_id and motor_id as explicit JSON null values.
    bool create(const OutgoingWindingJob& job, uint32_t createdUptimeMs);

    // Creates one immutable snapshot linked to an existing repair and motor.
    // Both identifiers must be non-zero; partial linkage is rejected.
    bool create(const OutgoingWindingJob& job,
                const JobLinkage& linkage,
                uint32_t createdUptimeMs);
    bool exists(uint32_t sessionId) const;

    // Loads and validates immutable program and linkage data for recovery.
    // Legacy schema-v1 snapshots without repeat_target are read as target 1.
    bool load(uint32_t sessionId, JobSnapshot& snapshot) const;

    // Verifies that the persisted immutable snapshot is structurally valid and
    // belongs to the exact job/session pair expected by runtime recovery.
    bool validateIdentity(uint32_t jobId, uint32_t sessionId) const;

private:
    static constexpr const char* RootDirectory = "/data/winding-jobs";
    static constexpr const char* SnapshotDirectory = "/data/winding-jobs/snapshots";
    static constexpr uint16_t MaxTurnsPerCoil = 9999U;

    fs::FS& m_fileSystem;
    bool m_ready;

    bool ensureDirectories();
    String snapshotPath(uint32_t sessionId) const;
    String temporaryPath(uint32_t sessionId) const;
    String serialize(const OutgoingWindingJob& job,
                     const JobLinkage& linkage,
                     uint32_t createdUptimeMs) const;
    bool readAndParse(const char* path, JobSnapshot& snapshot) const;
    static bool parse(const String& content, JobSnapshot& snapshot);
    static bool findUnsigned(const String& input,
                             const char* key,
                             uint32_t& value);
    static bool findOptionalUnsigned(const String& input,
                                     const char* key,
                                     bool& hasValue,
                                     uint32_t& value);
    static bool findNullableUnsigned(const String& input,
                                     const char* key,
                                     bool& hasValue,
                                     uint32_t& value);
    static bool findString(const String& input,
                           const char* key,
                           String& value);
    static bool findTurns(const String& input,
                          uint8_t expectedCount,
                          uint16_t* turns);
};
}
