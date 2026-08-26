#include "CM_JobLinkageResolver.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
JobLinkageResolver::JobLinkageResolver(fs::FS& storage)
    : m_storage(storage),
      m_windingVersions(storage),
      m_ready(false),
      m_windingVersionsReady(false)
{
}

bool JobLinkageResolver::begin()
{
    // A mounted but empty workshop is valid. Data files may be created later
    // by RepairRegistry during the same boot, so readiness must not depend on
    // /data already existing when this component starts.
    m_ready = false;
    m_windingVersionsReady = false;
    File root = m_storage.open("/", FILE_READ);
    if (!root) return false;
    root.close();
    m_windingVersionsReady = m_windingVersions.begin();
    if (!m_windingVersionsReady) return false;
    m_ready = true;
    return true;
}

bool JobLinkageResolver::isReady() const
{
    if (!m_ready || !m_windingVersionsReady || !m_windingVersions.ready())
        return false;
    File root = m_storage.open("/", FILE_READ);
    if (!root) return false;
    root.close();
    return true;
}

bool JobLinkageResolver::resolve(uint32_t repairId,
                                 uint32_t requestedMotorId,
                                 JobLinkage& linkage) const
{
    linkage = JobLinkage();
    if (!isReady() || repairId == 0UL || requestedMotorId == 0UL ||
        !m_storage.exists(RepairsPath))
    {
        return false;
    }

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    bool found = false;
    uint32_t storedMotorId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId))
        {
            file.close();
            return false;
        }
        if (candidateRepairId != repairId) continue;

        if (found || !findUnsigned(line, "motor_id", storedMotorId) ||
            storedMotorId == 0UL)
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();

    if (!found || storedMotorId != requestedMotorId || !repairIsOpen(repairId))
        return false;

    linkage.linked = true;
    linkage.repairId = repairId;
    linkage.motorId = storedMotorId;
    return linkage.isValid();
}

bool JobLinkageResolver::resolveWithProgram(uint32_t repairId,
                                            uint32_t requestedMotorId,
                                            JobLinkage& linkage,
                                            String& coilProgram) const
{
    return resolveWithProgram(repairId,
                              requestedMotorId,
                              RemoteJobType::Working,
                              linkage,
                              coilProgram);
}

bool JobLinkageResolver::resolveWithProgram(uint32_t repairId,
                                            uint32_t requestedMotorId,
                                            RemoteJobType requestedType,
                                            JobLinkage& linkage,
                                            String& coilProgram) const
{
    uint16_t ignoredRepeatTarget = 1U;
    return resolveWithProgramAndRepeat(repairId,
                                       requestedMotorId,
                                       requestedType,
                                       linkage,
                                       coilProgram,
                                       ignoredRepeatTarget);
}

bool JobLinkageResolver::resolveWithProgramAndRepeat(uint32_t repairId,
                                                     uint32_t requestedMotorId,
                                                     RemoteJobType requestedType,
                                                     JobLinkage& linkage,
                                                     String& coilProgram,
                                                     uint16_t& repeatTarget) const
{
    coilProgram = String();
    repeatTarget = 0U;
    if (!resolve(repairId, requestedMotorId, linkage))
    {
        linkage = JobLinkage();
        return false;
    }

    String versionJson;
    versionJson.reserve(960U);
    bool versionFound = false;
    if (!m_windingVersions.appendLatestByMotorJson(versionJson,
                                                    requestedMotorId,
                                                    versionFound))
    {
        linkage = JobLinkage();
        return false;
    }

    if (versionFound)
    {
        String candidateProgram;
        String canonicalProgram;
        uint32_t parsedRepeatTarget = 0UL;
        if (requestedType == RemoteJobType::Starting)
        {
            if (versionJson.indexOf(F("\"starting_present\":true")) < 0 ||
                !findString(versionJson, "starting_program", candidateProgram) ||
                !findUnsigned(versionJson, "starting_repeat_target", parsedRepeatTarget))
            {
                linkage = JobLinkage();
                return false;
            }
        }
        else if (!findString(versionJson, "working_program", candidateProgram) ||
                 !findUnsigned(versionJson, "working_repeat_target", parsedRepeatTarget))
        {
            linkage = JobLinkage();
            return false;
        }

        if (!WindingProgramParser::canonicalize(candidateProgram, canonicalProgram) ||
            parsedRepeatTarget == 0UL || parsedRepeatTarget > 0xFFFFUL)
        {
            linkage = JobLinkage();
            return false;
        }
        coilProgram = canonicalProgram;
        repeatTarget = static_cast<uint16_t>(parsedRepeatTarget);
        return true;
    }

    if (requestedType == RemoteJobType::Starting)
    {
        linkage = JobLinkage();
        return false;
    }
    return resolveLegacyWorkingProgram(requestedMotorId,
                                       linkage,
                                       coilProgram,
                                       repeatTarget);
}

bool JobLinkageResolver::resolveLegacyWorkingProgram(uint32_t requestedMotorId,
                                                      JobLinkage& linkage,
                                                      String& coilProgram,
                                                      uint16_t& repeatTarget) const
{
    coilProgram = String();
    repeatTarget = 0U;
    if (!m_storage.exists(MotorsPath))
    {
        linkage = JobLinkage();
        return false;
    }

    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        linkage = JobLinkage();
        return false;
    }

    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateMotorId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "motor_id", candidateMotorId))
        {
            file.close();
            linkage = JobLinkage();
            return false;
        }
        if (candidateMotorId != requestedMotorId) continue;

        String candidateProgram;
        String canonicalProgram;
        String status;
        uint32_t parsedRepeatTarget = 1UL;
        const bool hasRepeatTarget = findUnsigned(line, "repeat_target", parsedRepeatTarget);
        if (found ||
            !findString(line, "coil_program", candidateProgram) ||
            !WindingProgramParser::canonicalize(candidateProgram, canonicalProgram) ||
            (findString(line, "status", status) && status != "ACTIVE") ||
            (hasRepeatTarget && (parsedRepeatTarget == 0UL || parsedRepeatTarget > 0xFFFFUL)))
        {
            file.close();
            linkage = JobLinkage();
            coilProgram = String();
            repeatTarget = 0U;
            return false;
        }

        coilProgram = canonicalProgram;
        repeatTarget = static_cast<uint16_t>(parsedRepeatTarget);
        found = true;
    }
    file.close();

    if (!found)
    {
        linkage = JobLinkage();
        coilProgram = String();
        repeatTarget = 0U;
        return false;
    }
    return true;
}

bool JobLinkageResolver::repairIsOpen(uint32_t repairId) const
{
    if (repairId == 0UL) return false;
    if (!m_storage.exists(RepairStatusPath)) return true;

    File file = m_storage.open(RepairStatusPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    bool closed = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateRepairId = 0UL;
        String status;
        String closedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId) ||
            !findString(line, "status", status) || status != "CLOSED" ||
            !findString(line, "closed_at", closedAt) || closedAt.length() < 10U)
        {
            file.close();
            return false;
        }

        if (candidateRepairId != repairId) continue;
        if (closed)
        {
            file.close();
            return false;
        }
        closed = true;
    }

    file.close();
    return !closed;
}

bool JobLinkageResolver::findUnsigned(const String& line,
                                      const char* key,
                                      uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int position = line.indexOf(marker);
    if (position < 0 ||
        line.indexOf(marker, position + marker.length()) >= 0)
    {
        return false;
    }

    int cursor = position + marker.length();
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() &&
        isDigit(line[cursor + 1]))
    {
        return false;
    }

    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    while (cursor < line.length() && line[cursor] == ' ') ++cursor;
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}') ||
        parsed == 0UL)
    {
        return false;
    }

    value = parsed;
    return true;
}

bool JobLinkageResolver::findString(const String& line,
                                    const char* key,
                                    String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int position = line.indexOf(marker);
    if (position < 0 ||
        line.indexOf(marker, position + marker.length()) >= 0)
    {
        return false;
    }

    int cursor = position + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"')
        {
            while (cursor < line.length() && line[cursor] == ' ') ++cursor;
            return cursor < line.length() &&
                   (line[cursor] == ',' || line[cursor] == '}');
        }
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}
}
