#include "CM_RepairRegistry.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool RepairRegistry::appendClientByIdJson(String& json,
                                          uint32_t clientId,
                                          bool& found) const
{
    found = false;
    if (!ready() || clientId == 0UL) return false;
    if (!m_storage.exists(ClientsPath)) return true;

    File file = m_storage.open(ClientsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "client_id", candidateId) ||
            candidateId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateId != clientId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        json += line;
        found = true;
    }

    file.close();
    return true;
}

bool RepairRegistry::appendMotorByIdJson(String& json,
                                         uint32_t motorId,
                                         bool& found) const
{
    found = false;
    if (!ready() || motorId == 0UL) return false;
    if (!m_storage.exists(MotorsPath)) return true;

    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "motor_id", candidateId) ||
            candidateId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateId != motorId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        json += line;
        found = true;
    }

    file.close();
    return true;
}

bool RepairRegistry::appendRepairByIdJson(String& json,
                                          uint32_t repairId,
                                          bool& found) const
{
    found = false;
    if (!ready() || repairId == 0UL) return false;
    if (!m_storage.exists(RepairsPath)) return true;

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    String matchedLine;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateId) ||
            candidateId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateId != repairId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        matchedLine = line;
        found = true;
    }
    file.close();

    if (!found) return true;

    bool closed = false;
    String closedAt;
    if (!repairClosed(repairId, closed, closedAt)) return false;

    if (matchedLine.length() == 0U || matchedLine[matchedLine.length() - 1U] != '}')
        return false;
    matchedLine.remove(matchedLine.length() - 1U);
    matchedLine += F(",\"current_status\":\"");
    matchedLine += closed ? F("CLOSED") : F("OPEN");
    matchedLine += F("\",\"closed_at\":");
    if (closed)
    {
        matchedLine += '"';
        matchedLine += jsonEscape(closedAt);
        matchedLine += '"';
    }
    else
    {
        matchedLine += F("null");
    }
    matchedLine += '}';
    json += matchedLine;
    return true;
}
}
