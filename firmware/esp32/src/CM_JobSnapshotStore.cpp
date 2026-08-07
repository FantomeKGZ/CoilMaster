#include "CM_JobSnapshotStore.h"

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
        !content.endsWith(F("}\n")))
    {
        return false;
    }

    const String jobMarker = String(F("\"job_id\":")) + jobId + ',';
    const String sessionMarker =
        String(F("\"session_id\":")) + sessionId + ',';
    return content.indexOf(jobMarker) >= 0 &&
           content.indexOf(sessionMarker) >= 0;
}
}
