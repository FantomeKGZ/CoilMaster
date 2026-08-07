#include "CM_JobSnapshotStore.h"

#include <stdlib.h>

namespace CM
{
JobSnapshotStore::JobSnapshotStore(fs::FS& fileSystem)
    : m_fileSystem(fileSystem), m_ready(false)
{
}

bool JobSnapshotStore::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool JobSnapshotStore::isReady() const
{
    return m_ready;
}

bool JobSnapshotStore::create(const OutgoingWindingJob& job,
                              uint32_t createdUptimeMs)
{
    if (!m_ready || !job.isValid()) return false;

    const String finalPath = snapshotPath(job.sessionId);
    const String tempPath = temporaryPath(job.sessionId);
    if (m_fileSystem.exists(finalPath) || m_fileSystem.exists(tempPath))
        return false;

    const String record = serialize(job, createdUptimeMs);
    File file = m_fileSystem.open(tempPath, FILE_WRITE);
    if (!file) return false;

    const size_t written = file.print(record);
    file.flush();
    file.close();
    if (written != record.length() ||
        !verifySnapshot(tempPath.c_str(), job.jobId, job.sessionId))
    {
        m_fileSystem.remove(tempPath);
        return false;
    }

    if (!m_fileSystem.rename(tempPath, finalPath))
    {
        m_fileSystem.remove(tempPath);
        return false;
    }

    return verifySnapshot(finalPath.c_str(), job.jobId, job.sessionId);
}

bool JobSnapshotStore::exists(uint32_t sessionId) const
{
    return sessionId != 0UL && m_fileSystem.exists(snapshotPath(sessionId));
}

bool JobSnapshotStore::validateIdentity(uint32_t jobId,
                                        uint32_t sessionId) const
{
    if (!m_ready || jobId == 0UL || sessionId == 0UL) return false;
    const String path = snapshotPath(sessionId);
    return m_fileSystem.exists(path) &&
           verifySnapshot(path.c_str(), jobId, sessionId);
}

bool JobSnapshotStore::ensureDirectories()
{
    if (!m_fileSystem.exists("/data") && !m_fileSystem.mkdir("/data"))
        return false;

    if (!m_fileSystem.exists(RootDirectory) &&
        !m_fileSystem.mkdir(RootDirectory))
    {
        return false;
    }

    if (!m_fileSystem.exists(SnapshotDirectory) &&
        !m_fileSystem.mkdir(SnapshotDirectory))
    {
        return false;
    }

    return true;
}

String JobSnapshotStore::snapshotPath(uint32_t sessionId) const
{
    String path = F("/data/winding-jobs/snapshots/session-");
    path += sessionId;
    path += F(".json");
    return path;
}

String JobSnapshotStore::temporaryPath(uint32_t sessionId) const
{
    String path = snapshotPath(sessionId);
    path += F(".tmp");
    return path;
}

String JobSnapshotStore::serialize(const OutgoingWindingJob& job,
                                   uint32_t createdUptimeMs) const
{
    String result;
    result.reserve(384U);
    result = F("{\"schema_version\":1,\"job_id\":");
    result += job.jobId;
    result += F(",\"session_id\":");
    result += job.sessionId;
    result += F(",\"repair_id\":null,\"motor_id\":null,\"program_type\":\"");
    result += job.type == RemoteJobType::Starting ? F("STARTING") : F("WORKING");
    result += F("\",\"coil_count\":");
    result += job.coilCount;
    result += F(",\"turns\":[");
    for (uint8_t index = 0U; index < job.coilCount; ++index)
    {
        if (index > 0U) result += ',';
        result += job.turns[index];
    }
    result += F("],\"wire_type\":null,\"wire_diameter\":null,");
    result += F("\"created_at\":null,\"created_uptime_ms\":");
    result += createdUptimeMs;
    result += F(",\"delivery_state\":\"CREATED\",");
    result += F("\"execution_state\":\"WAITING_DELIVERY\"}\n");
    return result;
}

bool JobSnapshotStore::verifySnapshot(const char* path,
                                      uint32_t jobId,
                                      uint32_t sessionId) const
{
    File file = m_fileSystem.open(path, FILE_READ);
    if (!file) return false;

    const String content = file.readString();
    file.close();

    if (!content.startsWith(F("{\"schema_version\":1,")) ||
        !content.endsWith(F("}\n")) ||
        content.length() > 1024U)
    {
        return false;
    }

    uint32_t storedJobId = 0UL;
    uint32_t storedSessionId = 0UL;
    uint32_t coilCount = 0UL;
    uint32_t createdUptimeMs = 0UL;
    if (!findUnsigned(content, "job_id", storedJobId) ||
        !findUnsigned(content, "session_id", storedSessionId) ||
        !findUnsigned(content, "coil_count", coilCount) ||
        !findUnsigned(content, "created_uptime_ms", createdUptimeMs) ||
        storedJobId != jobId ||
        storedSessionId != sessionId ||
        coilCount == 0UL || coilCount > 10UL)
    {
        return false;
    }

    return content.indexOf(F("\"program_type\":\"WORKING\"")) >= 0 ||
           content.indexOf(F("\"program_type\":\"STARTING\"")) >= 0;
}

bool JobSnapshotStore::findUnsigned(const String& input,
                                    const char* key,
                                    uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    int start = input.indexOf(marker);
    if (start < 0) return false;
    start += marker.length();

    int end = start;
    while (end < input.length() && isDigit(input[end])) ++end;
    if (end == start) return false;

    const String number = input.substring(start, end);
    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(number.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0') return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}
}
