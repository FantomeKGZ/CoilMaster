#include "CM_MotorWindingVersionStore.h"

namespace CM
{
namespace
{
bool prepareAutonomousNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}
}

bool MotorWindingVersionStore::analyzeAutonomousProjection(
    uint32_t sessionId,
    uint32_t runId,
    const String& role,
    uint32_t targetMotorId,
    uint32_t& projectedMotorId,
    uint32_t& projectedVersionId,
    bool& projectionFound,
    NewMotorWindingVersion& latest,
    uint32_t& latestVersionId,
    bool& latestFound) const
{
    projectedMotorId = 0UL;
    projectedVersionId = 0UL;
    projectionFound = false;
    latest = NewMotorWindingVersion();
    latestVersionId = 0UL;
    latestFound = false;

    if (!ready() || sessionId == 0UL || runId == 0UL || targetMotorId == 0UL ||
        (role != "WORKING" && role != "STARTING"))
    {
        return false;
    }
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareAutonomousNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentVersionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", currentVersionId) ||
            currentVersionId == 0UL || currentVersionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = currentVersionId;

        uint32_t sourceSessionId = 0UL;
        uint32_t sourceRunId = 0UL;
        bool sessionPresent = false;
        bool runPresent = false;
        if (!findOptionalUnsigned(line, "source_autonomous_session_id",
                                  sourceSessionId, sessionPresent) ||
            !findOptionalUnsigned(line, "source_autonomous_run_id",
                                  sourceRunId, runPresent))
        {
            file.close();
            return false;
        }

        String sourceRole;
        const bool rolePresent = line.indexOf(F("\"source_autonomous_role\":")) >= 0;
        if (sessionPresent || runPresent || rolePresent)
        {
            if (!sessionPresent || !runPresent || !rolePresent ||
                sourceSessionId == 0UL || sourceRunId == 0UL ||
                !findString(line, "source_autonomous_role", sourceRole, true) ||
                (sourceRole != "WORKING" && sourceRole != "STARTING"))
            {
                file.close();
                return false;
            }

            if (sourceSessionId == sessionId && sourceRunId == runId &&
                sourceRole == role)
            {
                if (projectionFound)
                {
                    file.close();
                    return false;
                }
                projectionFound = true;
                projectedMotorId = currentMotorId;
                projectedVersionId = currentVersionId;
            }
        }

        if (currentMotorId != targetMotorId) continue;

        NewMotorWindingVersion parsed;
        uint32_t parsedVersionId = 0UL;
        if (!parseVersion(line, parsed, parsedVersionId) ||
            parsedVersionId != currentVersionId || parsed.motorId != targetMotorId)
        {
            file.close();
            return false;
        }
        latest = parsed;
        latestVersionId = currentVersionId;
        latestFound = true;
    }

    file.close();
    return true;
}
}
