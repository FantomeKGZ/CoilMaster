#include "CM_PersistentIdAllocator.h"

#include <stdlib.h>
#include <string.h>

namespace CM
{
PersistentIdAllocator::PersistentIdAllocator(fs::FS& fileSystem)
    : m_fileSystem(fileSystem),
      m_lastJobId(0UL),
      m_lastSessionId(0UL),
      m_ready(false)
{
}

bool PersistentIdAllocator::begin()
{
    m_ready = false;
    m_lastJobId = 0UL;
    m_lastSessionId = 0UL;

    if (!ensureDirectories()) return false;

    if (!m_fileSystem.exists(StatePath))
    {
        // A missing state file is valid only for first initialization.
        // Persist zero as the last allocated value before serving requests.
        if (!persistState(0UL, 0UL)) return false;
        m_ready = true;
        return true;
    }

    uint32_t jobId = 0UL;
    uint32_t sessionId = 0UL;
    if (!loadState(StatePath, jobId, sessionId))
    {
        // Recover only from a complete valid backup. Never silently reset IDs.
        if (!m_fileSystem.exists(BackupPath) ||
            !loadState(BackupPath, jobId, sessionId) ||
            !persistState(jobId, sessionId))
        {
            return false;
        }
    }

    m_lastJobId = jobId;
    m_lastSessionId = sessionId;
    m_ready = true;
    return true;
}

bool PersistentIdAllocator::isReady() const
{
    return m_ready;
}

bool PersistentIdAllocator::allocate(uint32_t& jobId, uint32_t& sessionId)
{
    jobId = 0UL;
    sessionId = 0UL;

    if (!m_ready ||
        m_lastJobId == 0xFFFFFFFFUL ||
        m_lastSessionId == 0xFFFFFFFFUL)
    {
        return false;
    }

    const uint32_t candidateJobId = m_lastJobId + 1UL;
    const uint32_t candidateSessionId = m_lastSessionId + 1UL;
    if (candidateJobId == 0UL || candidateSessionId == 0UL ||
        !persistState(candidateJobId, candidateSessionId))
    {
        m_ready = false;
        return false;
    }

    m_lastJobId = candidateJobId;
    m_lastSessionId = candidateSessionId;
    jobId = candidateJobId;
    sessionId = candidateSessionId;
    return true;
}

uint32_t PersistentIdAllocator::lastJobId() const
{
    return m_lastJobId;
}

uint32_t PersistentIdAllocator::lastSessionId() const
{
    return m_lastSessionId;
}

bool PersistentIdAllocator::ensureDirectories()
{
    if (!m_fileSystem.exists("/data") && !m_fileSystem.mkdir("/data"))
        return false;

    if (!m_fileSystem.exists(DirectoryPath) &&
        !m_fileSystem.mkdir(DirectoryPath))
    {
        return false;
    }

    return true;
}

bool PersistentIdAllocator::loadState(const char* path,
                                      uint32_t& jobId,
                                      uint32_t& sessionId) const
{
    jobId = 0UL;
    sessionId = 0UL;

    File file = m_fileSystem.open(path, FILE_READ);
    if (!file) return false;

    const String schemaLine = file.readStringUntil('\n');
    const String jobLine = file.readStringUntil('\n');
    const String sessionLine = file.readStringUntil('\n');
    const String extraLine = file.readStringUntil('\n');
    file.close();

    uint32_t schemaVersion = 0UL;
    if (!parseUnsignedLine(schemaLine, "schema_version", schemaVersion) ||
        schemaVersion != 1UL ||
        !parseUnsignedLine(jobLine, "last_job_id", jobId) ||
        !parseUnsignedLine(sessionLine, "last_session_id", sessionId) ||
        extraLine.length() != 0U)
    {
        return false;
    }

    // The two counters are allocated together and therefore must stay equal.
    return jobId == sessionId;
}

bool PersistentIdAllocator::persistState(uint32_t jobId, uint32_t sessionId)
{
    if (jobId != sessionId) return false;

    if (m_fileSystem.exists(TempPath) && !m_fileSystem.remove(TempPath))
        return false;

    File temp = m_fileSystem.open(TempPath, FILE_WRITE);
    if (!temp) return false;

    String record;
    record.reserve(80U);
    record = F("schema_version=1\nlast_job_id=");
    record += jobId;
    record += F("\nlast_session_id=");
    record += sessionId;
    record += '\n';

    const size_t written = temp.print(record);
    temp.flush();
    temp.close();
    if (written != record.length())
    {
        m_fileSystem.remove(TempPath);
        return false;
    }

    uint32_t verifiedJobId = 0UL;
    uint32_t verifiedSessionId = 0UL;
    if (!loadState(TempPath, verifiedJobId, verifiedSessionId) ||
        verifiedJobId != jobId || verifiedSessionId != sessionId)
    {
        m_fileSystem.remove(TempPath);
        return false;
    }

    if (m_fileSystem.exists(BackupPath) && !m_fileSystem.remove(BackupPath))
    {
        m_fileSystem.remove(TempPath);
        return false;
    }

    if (m_fileSystem.exists(StatePath) &&
        !m_fileSystem.rename(StatePath, BackupPath))
    {
        m_fileSystem.remove(TempPath);
        return false;
    }

    if (!m_fileSystem.rename(TempPath, StatePath))
    {
        // Best-effort rollback. Failure leaves begin() able to recover from backup.
        if (!m_fileSystem.exists(StatePath) && m_fileSystem.exists(BackupPath))
            m_fileSystem.rename(BackupPath, StatePath);
        return false;
    }

    return true;
}

bool PersistentIdAllocator::parseUnsignedLine(const String& line,
                                              const char* key,
                                              uint32_t& value)
{
    value = 0UL;
    const String prefix = String(key) + '=';
    if (!line.startsWith(prefix)) return false;

    const String number = line.substring(prefix.length());
    if (number.length() == 0U) return false;

    uint64_t parsed = 0ULL;
    for (size_t index = 0U; index < number.length(); ++index)
    {
        const char character = number[index];
        if (character < '0' || character > '9') return false;
        parsed = parsed * 10ULL + static_cast<uint8_t>(character - '0');
        if (parsed > 0xFFFFFFFFULL) return false;
    }

    value = static_cast<uint32_t>(parsed);
    return true;
}
}
