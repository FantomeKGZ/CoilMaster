#pragma once

#include <Arduino.h>
#include <FS.h>

#include "CM_UartEventReceiver.h"

namespace CM
{
struct JobSnapshot
{
    static constexpr uint8_t MaxCoils = 10U;

    uint32_t jobId;
    uint32_t sessionId;
    RemoteJobType type;
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

    // Creates one immutable snapshot. Existing session files are never replaced.
    bool create(const OutgoingWindingJob& job, uint32_t createdUptimeMs);
    bool exists(uint32_t sessionId) const;

    // Loads and validates immutable program data for display/recovery only.
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
                     uint32_t createdUptimeMs) const;
    bool readAndParse(const char* path, JobSnapshot& snapshot) const;
    static bool parse(const String& content, JobSnapshot& snapshot);
    static bool findUnsigned(const String& input,
                             const char* key,
                             uint32_t& value);
    static bool findString(const String& input,
                           const char* key,
                           String& value);
    static bool findTurns(const String& input,
                          uint8_t expectedCount,
                          uint16_t* turns);
};
}
