#include "CM_JobSnapshotStore.h"

#include <stdlib.h>

namespace CM
{
JobSnapshot::JobSnapshot()
    : jobId(0UL),
      sessionId(0UL),
      type(RemoteJobType::Working),
      coilCount(0U),
      turns{},
      createdUptimeMs(0UL)
{
}

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

    JobSnapshot verified;
    if (written != record.length() ||
        !readAndParse(tempPath.c_str(), verified) ||
        verified.jobId != job.jobId ||
        verified.sessionId != job.sessionId)
    {
        m_fileSystem.remove(tempPath);
        return false;
    }

    if (!m_fileSystem.rename(tempPath, finalPath))
    {
        m_fileSystem.remove(tempPath);
        return false;
    }

    if (!readAndParse(finalPath.c_str(), verified)) return false;
    return verified.jobId == job.jobId && verified.sessionId == job.sessionId;
}

bool JobSnapshotStore::exists(uint32_t sessionId) const
{
    return sessionId != 0UL && m_fileSystem.exists(snapshotPath(sessionId));
}

bool JobSnapshotStore::load(uint32_t sessionId,
                            JobSnapshot& snapshot) const
{
    snapshot = JobSnapshot();
    if (!m_ready || sessionId == 0UL) return false;

    const String path = snapshotPath(sessionId);
    return m_fileSystem.exists(path) &&
           readAndParse(path.c_str(), snapshot) &&
           snapshot.sessionId == sessionId;
}

bool JobSnapshotStore::validateIdentity(uint32_t jobId,
                                        uint32_t sessionId) const
{
    JobSnapshot snapshot;
    return jobId != 0UL &&
           load(sessionId, snapshot) &&
           snapshot.jobId == jobId;
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

bool JobSnapshotStore::readAndParse(const char* path,
                                    JobSnapshot& snapshot) const
{
    snapshot = JobSnapshot();
    File file = m_fileSystem.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    if (file.size() == 0U || file.size() > 1024U)
    {
        file.close();
        return false;
    }

    const String content = file.readString();
    file.close();
    return parse(content, snapshot);
}

bool JobSnapshotStore::parse(const String& content,
                             JobSnapshot& snapshot)
{
    snapshot = JobSnapshot();
    if (!content.startsWith(F("{\"schema_version\":1,")) ||
        !content.endsWith(F("}\n")) ||
        content.length() > 1024U)
    {
        return false;
    }

    uint32_t schemaVersion = 0UL;
    uint32_t coilCount = 0UL;
    String programType;
    if (!findUnsigned(content, "schema_version", schemaVersion) ||
        schemaVersion != 1UL ||
        !findUnsigned(content, "job_id", snapshot.jobId) ||
        snapshot.jobId == 0UL ||
        !findUnsigned(content, "session_id", snapshot.sessionId) ||
        snapshot.sessionId == 0UL ||
        !findString(content, "program_type", programType) ||
        !findUnsigned(content, "coil_count", coilCount) ||
        coilCount == 0UL || coilCount > JobSnapshot::MaxCoils ||
        !findUnsigned(content, "created_uptime_ms", snapshot.createdUptimeMs))
    {
        return false;
    }

    if (programType == "WORKING")
        snapshot.type = RemoteJobType::Working;
    else if (programType == "STARTING")
        snapshot.type = RemoteJobType::Starting;
    else
        return false;

    snapshot.coilCount = static_cast<uint8_t>(coilCount);
    return findTurns(content, snapshot.coilCount, snapshot.turns);
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

bool JobSnapshotStore::findString(const String& input,
                                  const char* key,
                                  String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    int start = input.indexOf(marker);
    if (start < 0) return false;
    start += marker.length();

    const int end = input.indexOf('"', start);
    if (end < 0 || end == start) return false;
    value = input.substring(start, end);
    return true;
}

bool JobSnapshotStore::findTurns(const String& input,
                                 uint8_t expectedCount,
                                 uint16_t* turns)
{
    if (expectedCount == 0U || turns == nullptr) return false;

    const String marker = F("\"turns\":[");
    int start = input.indexOf(marker);
    if (start < 0) return false;
    start += marker.length();

    const int arrayEnd = input.indexOf(']', start);
    if (arrayEnd < 0) return false;

    uint8_t count = 0U;
    int cursor = start;
    while (cursor < arrayEnd)
    {
        if (count >= expectedCount) return false;

        int valueEnd = cursor;
        while (valueEnd < arrayEnd && isDigit(input[valueEnd])) ++valueEnd;
        if (valueEnd == cursor) return false;

        const String number = input.substring(cursor, valueEnd);
        char* parseEnd = nullptr;
        const unsigned long parsed = strtoul(number.c_str(), &parseEnd, 10);
        if (parseEnd == nullptr || *parseEnd != '\0' ||
            parsed == 0UL || parsed > MaxTurnsPerCoil)
        {
            return false;
        }

        turns[count++] = static_cast<uint16_t>(parsed);
        if (valueEnd == arrayEnd) break;
        if (input[valueEnd] != ',') return false;
        cursor = valueEnd + 1;
    }

    return count == expectedCount;
}
}
