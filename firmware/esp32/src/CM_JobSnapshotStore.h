#pragma once

#include <Arduino.h>
#include <FS.h>

#include "CM_UartEventReceiver.h"

namespace CM
{
class JobSnapshotStore
{
public:
    explicit JobSnapshotStore(fs::FS& fileSystem);

    bool begin();
    bool isReady() const;

    // Creates one immutable snapshot. Existing session files are never replaced.
    bool create(const OutgoingWindingJob& job, uint32_t createdUptimeMs);
    bool exists(uint32_t sessionId) const;

    // Verifies that the persisted immutable snapshot is structurally valid and
    // belongs to the exact job/session pair expected by runtime recovery.
    bool validateIdentity(uint32_t jobId, uint32_t sessionId) const;

private:
    static constexpr const char* RootDirectory = "/data/winding-jobs";
    static constexpr const char* SnapshotDirectory = "/data/winding-jobs/snapshots";

    fs::FS& m_fileSystem;
    bool m_ready;

    bool ensureDirectories();
    String snapshotPath(uint32_t sessionId) const;
    String temporaryPath(uint32_t sessionId) const;
    String serialize(const OutgoingWindingJob& job,
                     uint32_t createdUptimeMs) const;
    bool verifySnapshot(const char* path,
                        uint32_t jobId,
                        uint32_t sessionId) const;
    static bool findUnsigned(const String& input,
                             const char* key,
                             uint32_t& value);
};
}
