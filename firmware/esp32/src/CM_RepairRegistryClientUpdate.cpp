#include "CM_RepairRegistry.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool RepairRegistry::updateClient(uint32_t clientId, const NewClient& client)
{
    const String normalized = normalizePhone(client.phone);
    bool found = false;
    if (!ready() || clientId == 0UL || client.name.length() == 0U ||
        normalized.length() < 7U ||
        !clientExists(clientId, found) || !found)
    {
        return false;
    }

    File file = m_storage.open(ClientRevisionsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(320U);
    line = F("{\"client_id\":"); line += clientId;
    line += F(",\"name\":\""); line += jsonEscape(client.name);
    line += F("\",\"phone\":\""); line += jsonEscape(client.phone);
    line += F("\",\"phone_normalized\":\""); line += normalized;
    line += F("\",\"status\":\"ACTIVE\"");
    if (client.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(client.comment);
        line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::latestClientRevisionLine(uint32_t clientId,
                                              String& line,
                                              bool& found) const
{
    line = String();
    found = false;
    if (!ready() || clientId == 0UL) return false;
    if (!m_storage.exists(ClientRevisionsPath)) return true;

    File file = m_storage.open(ClientRevisionsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL ||
        (rawSize > 0U &&
         (!file.seek(static_cast<uint32_t>(rawSize - 1U)) ||
          file.read() != '\n' || !file.seek(0U))))
    {
        file.close();
        return false;
    }

    while (file.available())
    {
        const String candidate = file.readStringUntil('\n');
        if (candidate.length() == 0U) continue;
        uint32_t candidateId = 0UL;
        if (!FlatJsonObjectValidator::valid(candidate) ||
            !findUnsigned(candidate, "client_id", candidateId) ||
            candidateId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateId != clientId) continue;
        line = candidate;
        found = true;
    }

    file.close();
    return true;
}
}
