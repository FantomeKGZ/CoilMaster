#include "CM_JobLinkageResolver.h"

#include <stdlib.h>

namespace CM
{
JobLinkageResolver::JobLinkageResolver(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool JobLinkageResolver::begin()
{
    // An empty workshop is valid. The store becomes usable once /data exists;
    // resolve() will report unknown repairs until the repairs file is created.
    m_ready = m_storage.exists("/data");
    return m_ready;
}

bool JobLinkageResolver::isReady() const
{
    return m_ready;
}

bool JobLinkageResolver::resolve(uint32_t repairId,
                                 uint32_t requestedMotorId,
                                 JobLinkage& linkage) const
{
    linkage = JobLinkage();
    if (!m_ready || repairId == 0UL || requestedMotorId == 0UL ||
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
        if (!findUnsigned(line, "repair_id", candidateRepairId) ||
            candidateRepairId != repairId)
        {
            continue;
        }

        // Duplicate repair identifiers make the source ambiguous and unsafe.
        if (found || !findUnsigned(line, "motor_id", storedMotorId) ||
            storedMotorId == 0UL)
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();

    if (!found || storedMotorId != requestedMotorId) return false;

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
    coilProgram = String();
    if (!resolve(repairId, requestedMotorId, linkage) ||
        !m_storage.exists(MotorsPath))
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
        if (!findUnsigned(line, "motor_id", candidateMotorId) ||
            candidateMotorId != requestedMotorId)
        {
            continue;
        }

        String candidateProgram;
        String status;
        if (found ||
            !findString(line, "coil_program", candidateProgram) ||
            candidateProgram.length() == 0U ||
            (findString(line, "status", status) && status != "ACTIVE"))
        {
            file.close();
            linkage = JobLinkage();
            coilProgram = String();
            return false;
        }

        candidateProgram.trim();
        if (candidateProgram.length() == 0U)
        {
            file.close();
            linkage = JobLinkage();
            return false;
        }

        coilProgram = candidateProgram;
        found = true;
    }
    file.close();

    if (!found)
    {
        linkage = JobLinkage();
        coilProgram = String();
        return false;
    }
    return true;
}

bool JobLinkageResolver::findUnsigned(const String& line,
                                      const char* key,
                                      uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    int start = line.indexOf(marker);
    if (start < 0) return false;
    start += marker.length();

    int end = start;
    while (end < line.length() && isDigit(line[end])) ++end;
    if (end == start) return false;

    // A canonical JSON number must end at a field or object delimiter.
    if (end < line.length() && line[end] != ',' && line[end] != '}') return false;

    const String number = line.substring(start, end);
    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(number.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0' || parsed == 0UL)
        return false;

    value = static_cast<uint32_t>(parsed);
    return true;
}

bool JobLinkageResolver::findString(const String& line,
                                    const char* key,
                                    String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int position = line.indexOf(marker);
    if (position < 0) return false;

    const int start = position + marker.length();
    const int end = line.indexOf('"', start);
    if (end < 0) return false;

    value = line.substring(start, end);
    return true;
}
}
