#include "CM_RepairRegistry.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool seekPageCursor(File& file, uint32_t cursor, uint32_t& fileSize)
{
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL || cursor > rawSize) return false;
    fileSize = static_cast<uint32_t>(rawSize);

    if (cursor > 0UL)
    {
        if (!file.seek(cursor - 1UL) || file.read() != '\n') return false;
    }
    return file.seek(cursor);
}

void finishPage(File& file,
                uint32_t fileSize,
                uint32_t& nextCursor,
                bool& hasMore)
{
    const uint32_t pageEnd = static_cast<uint32_t>(file.position());
    hasMore = pageEnd < fileSize;
    nextCursor = hasMore ? pageEnd : 0UL;
}
}

bool RepairRegistry::appendClientsPageJson(String& json,
                                           const String& phoneQuery,
                                           uint32_t cursor,
                                           uint8_t limit,
                                           uint16_t& count,
                                           uint32_t& nextCursor,
                                           bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize) return false;
    if (!m_storage.exists(ClientsPath)) return cursor == 0UL;

    File file = m_storage.open(ClientsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t fileSize = 0UL;
    if (!seekPageCursor(file, cursor, fileSize))
    {
        file.close();
        return false;
    }

    const String query = normalizePhone(phoneQuery);
    bool first = true;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        String normalized;
        if (query.length() > 0U &&
            (!findString(line, "phone_normalized", normalized) ||
             normalized.indexOf(query) < 0))
        {
            continue;
        }

        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }

    finishPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}

bool RepairRegistry::appendMotorsPageJson(String& json,
                                          const String& searchQuery,
                                          uint32_t cursor,
                                          uint8_t limit,
                                          uint16_t& count,
                                          uint32_t& nextCursor,
                                          bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize) return false;
    if (!m_storage.exists(MotorsPath)) return cursor == 0UL;

    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t fileSize = 0UL;
    if (!seekPageCursor(file, cursor, fileSize))
    {
        file.close();
        return false;
    }

    String query = searchQuery;
    query.trim();
    query.toLowerCase();
    bool first = true;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        if (query.length() > 0U)
        {
            String searchable;
            String field;
            if (findString(line, "name", field)) searchable += field + ' ';
            if (findString(line, "model", field)) searchable += field + ' ';
            if (findString(line, "manufacturer", field)) searchable += field + ' ';
            if (findString(line, "tags", field)) searchable += field + ' ';
            if (findString(line, "coil_program", field)) searchable += field;
            searchable.toLowerCase();
            if (searchable.indexOf(query) < 0) continue;
        }

        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }

    finishPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}

bool RepairRegistry::resolveRepairPageStatuses(
    const uint32_t* repairIds,
    uint8_t repairCount,
    bool* closed,
    String* closedAt) const
{
    if (!ready() || repairIds == nullptr || closed == nullptr ||
        closedAt == nullptr || repairCount > MaxListPageSize)
    {
        return false;
    }

    for (uint8_t index = 0U; index < repairCount; ++index)
    {
        if (repairIds[index] == 0UL) return false;
        closed[index] = false;
        closedAt[index] = String();
    }
    if (repairCount == 0U || !m_storage.exists(RepairStatusPath)) return true;

    File statusFile = m_storage.open(RepairStatusPath, FILE_READ);
    if (!statusFile || statusFile.isDirectory())
    {
        if (statusFile) statusFile.close();
        return false;
    }
    const size_t rawSize = statusFile.size();
    if (rawSize > 0xFFFFFFFFUL)
    {
        statusFile.close();
        return false;
    }
    if (rawSize > 0U &&
        (!statusFile.seek(static_cast<uint32_t>(rawSize - 1U)) ||
         statusFile.read() != '\n' || !statusFile.seek(0U)))
    {
        statusFile.close();
        return false;
    }

    while (statusFile.available())
    {
        const String line = statusFile.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t candidateRepairId = 0UL;
        String status;
        String candidateClosedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", candidateRepairId) ||
            candidateRepairId == 0UL ||
            !findString(line, "status", status) || status != "CLOSED" ||
            !findString(line, "closed_at", candidateClosedAt) ||
            candidateClosedAt.length() < 10U)
        {
            statusFile.close();
            return false;
        }

        for (uint8_t index = 0U; index < repairCount; ++index)
        {
            if (repairIds[index] != candidateRepairId) continue;
            if (closed[index])
            {
                statusFile.close();
                return false;
            }
            closed[index] = true;
            closedAt[index] = candidateClosedAt;
            break;
        }
    }

    statusFile.close();
    return true;
}

bool RepairRegistry::appendRepairsPageJson(String& json,
                                           uint32_t clientId,
                                           uint32_t cursor,
                                           uint8_t limit,
                                           uint16_t& count,
                                           uint32_t& nextCursor,
                                           bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize) return false;
    if (!m_storage.exists(RepairsPath)) return cursor == 0UL;

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t fileSize = 0UL;
    if (!seekPageCursor(file, cursor, fileSize))
    {
        file.close();
        return false;
    }

    String pageLines[MaxListPageSize];
    uint32_t repairIds[MaxListPageSize];
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t repairId = 0UL;
        uint32_t lineClientId = 0UL;
        if (!findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", lineClientId) || lineClientId == 0UL)
        {
            file.close();
            return false;
        }
        if (clientId > 0UL && lineClientId != clientId) continue;

        pageLines[count] = line;
        repairIds[count] = repairId;
        ++count;
    }

    finishPage(file, fileSize, nextCursor, hasMore);
    file.close();

    bool closed[MaxListPageSize];
    String closedAt[MaxListPageSize];
    if (!resolveRepairPageStatuses(repairIds,
                                   static_cast<uint8_t>(count),
                                   closed,
                                   closedAt))
    {
        return false;
    }

    for (uint16_t index = 0U; index < count; ++index)
    {
        String& decorated = pageLines[index];
        decorated.remove(decorated.length() - 1U);
        decorated += F(",\"current_status\":\"");
        decorated += closed[index] ? F("CLOSED") : F("OPEN");
        decorated += F("\",\"closed_at\":");
        if (closed[index])
        {
            decorated += '"';
            decorated += jsonEscape(closedAt[index]);
            decorated += '"';
        }
        else
        {
            decorated += F("null");
        }
        decorated += '}';

        if (index > 0U) json += ',';
        json += decorated;
    }
    return true;
}
}
