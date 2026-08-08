#include "CM_PersistentIdIntegrityAudit.h"
#include <Arduino.h>

namespace CM
{
namespace
{
bool parseUnsignedLine(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String prefix = String(key) + '=';
    if (!line.startsWith(prefix)) return false;
    const String number = line.substring(prefix.length());
    if (number.length() == 0U) return false;

    uint64_t parsed = 0ULL;
    for (size_t i = 0U; i < number.length(); ++i)
    {
        const char ch = number[i];
        if (ch < '0' || ch > '9') return false;
        parsed = parsed * 10ULL + static_cast<uint8_t>(ch - '0');
        if (parsed > 0xFFFFFFFFULL) return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool loadState(fs::FS& storage, const char* path, uint32_t& value)
{
    value = 0UL;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const String schemaLine = file.readStringUntil('\n');
    const String jobLine = file.readStringUntil('\n');
    const String sessionLine = file.readStringUntil('\n');
    const String extraLine = file.readStringUntil('\n');
    file.close();

    uint32_t schemaVersion = 0UL, jobId = 0UL, sessionId = 0UL;
    if (!parseUnsignedLine(schemaLine, "schema_version", schemaVersion) || schemaVersion != 1UL ||
        !parseUnsignedLine(jobLine, "last_job_id", jobId) ||
        !parseUnsignedLine(sessionLine, "last_session_id", sessionId) ||
        extraLine.length() != 0U || jobId != sessionId)
    {
        return false;
    }
    value = jobId;
    return true;
}
}

bool PersistentIdIntegrityAudit::check(fs::FS& storage)
{
    PersistentIdIntegrityAuditMetrics ignoredMetrics;
    return check(storage, ignoredMetrics);
}

bool PersistentIdIntegrityAudit::check(fs::FS& storage,
                                       PersistentIdIntegrityAuditMetrics& metrics)
{
    constexpr const char* Directory = "/data/winding-jobs";
    constexpr const char* State = "/data/winding-jobs/id-state.txt";
    constexpr const char* Temp = "/data/winding-jobs/id-state.tmp";
    constexpr const char* Backup = "/data/winding-jobs/id-state.bak";

    metrics = PersistentIdIntegrityAuditMetrics();
    uint32_t validatedLastAllocatedId = 0UL;

    if (!storage.exists(Directory))
    {
        metrics.lastAllocatedId = validatedLastAllocatedId;
        return true;
    }
    File directory = storage.open(Directory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }
    directory.close();

    if (storage.exists(Temp) || !storage.exists(State)) return false;

    uint32_t mainValue = 0UL;
    if (!loadState(storage, State, mainValue)) return false;

    if (storage.exists(Backup))
    {
        uint32_t backupValue = 0UL;
        if (!loadState(storage, Backup, backupValue) || backupValue > mainValue)
            return false;
    }

    validatedLastAllocatedId = mainValue;
    metrics.lastAllocatedId = validatedLastAllocatedId;
    return true;
}
}
