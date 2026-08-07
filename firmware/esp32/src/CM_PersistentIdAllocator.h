#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
class PersistentIdAllocator
{
public:
    explicit PersistentIdAllocator(fs::FS& fileSystem);

    bool begin();
    bool isReady() const;

    // Allocates and persists both identifiers before returning them.
    // Skipped identifiers are acceptable; reuse is not.
    bool allocate(uint32_t& jobId, uint32_t& sessionId);

    uint32_t lastJobId() const;
    uint32_t lastSessionId() const;

private:
    static constexpr const char* DirectoryPath = "/data/winding-jobs";
    static constexpr const char* StatePath = "/data/winding-jobs/id-state.txt";
    static constexpr const char* TempPath = "/data/winding-jobs/id-state.tmp";
    static constexpr const char* BackupPath = "/data/winding-jobs/id-state.bak";

    fs::FS& m_fileSystem;
    uint32_t m_lastJobId;
    uint32_t m_lastSessionId;
    bool m_ready;

    bool ensureDirectories();
    bool loadState(const char* path, uint32_t& jobId, uint32_t& sessionId) const;
    bool persistState(uint32_t jobId, uint32_t sessionId);
    static bool parseUnsignedLine(const String& line,
                                  const char* key,
                                  uint32_t& value);
};
}
