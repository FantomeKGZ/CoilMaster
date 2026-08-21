#include "CM_RepairRegistry.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool seekSearchCursor(File& file, uint32_t cursor, uint32_t& fileSize)
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

void finishSearchPage(File& file,
                      uint32_t fileSize,
                      uint32_t& nextCursor,
                      bool& hasMore)
{
    const uint32_t pageEnd = static_cast<uint32_t>(file.position());
    hasMore = pageEnd < fileSize;
    nextCursor = hasMore ? pageEnd : 0UL;
}

bool containsQuery(String searchable, const String& query)
{
    searchable.toLowerCase();
    return searchable.indexOf(query) >= 0;
}
}

bool RepairRegistry::appendClientsSearchPageJson(String& json,
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
    if (!m_storage.exists(ClientsPath)) return cursor == 0UL;

    String query = searchQuery;
    query.trim();
    query.toLowerCase();
    if (query.length() == 0U) return false;

    File file = m_storage.open(ClientsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t fileSize = 0UL;
    if (!seekSearchCursor(file, cursor, fileSize))
    {
        file.close();
        return false;
    }

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

        uint32_t clientId = 0UL;
        String name;
        String phone;
        String normalized;
        String comment;
        if (!findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findString(line, "name", name) ||
            !findString(line, "phone", phone) ||
            !findString(line, "phone_normalized", normalized))
        {
            file.close();
            return false;
        }
        findString(line, "comment", comment);

        String searchable;
        searchable.reserve(name.length() + phone.length() + normalized.length() +
                           comment.length() + 24U);
        searchable += name;
        searchable += ' ';
        searchable += phone;
        searchable += ' ';
        searchable += normalized;
        searchable += ' ';
        searchable += comment;
        searchable += ' ';
        searchable += String(clientId);
        if (!containsQuery(searchable, query)) continue;

        if (!first) json += ',';
        first = false;
        json += line;
        ++count;
    }

    finishSearchPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}

bool RepairRegistry::appendRepairsSearchPageJson(String& json,
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
    if (!m_storage.exists(RepairsPath)) return cursor == 0UL;

    String query = searchQuery;
    query.trim();
    query.toLowerCase();
    if (query.length() == 0U) return false;

    File file = m_storage.open(RepairsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t fileSize = 0UL;
    if (!seekSearchCursor(file, cursor, fileSize))
    {
        file.close();
        return false;
    }

    bool first = true;
    bool pageFull = false;
    while (file.available() && !pageFull)
    {
        String pageLines[MaxListPageSize];
        uint32_t repairIds[MaxListPageSize];
        uint32_t recordStarts[MaxListPageSize];
        uint8_t batchCount = 0U;

        while (file.available() && batchCount < MaxListPageSize)
        {
            const uint32_t recordStart = static_cast<uint32_t>(file.position());
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;
            if (!FlatJsonObjectValidator::valid(line))
            {
                file.close();
                return false;
            }

            uint32_t repairId = 0UL;
            uint32_t clientId = 0UL;
            uint32_t motorId = 0UL;
            String receivedAt;
            String complaint;
            String comment;
            if (!findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
                !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
                !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
                !findString(line, "received_at", receivedAt))
            {
                file.close();
                return false;
            }
            findString(line, "complaint", complaint);
            findString(line, "comment", comment);

            String searchable;
            searchable.reserve(receivedAt.length() + complaint.length() +
                               comment.length() + 48U);
            searchable += String(repairId);
            searchable += F(" repair ремонт ");
            searchable += String(clientId);
            searchable += F(" client клиент ");
            searchable += String(motorId);
            searchable += F(" motor двигатель ");
            searchable += receivedAt;
            searchable += ' ';
            searchable += complaint;
            searchable += ' ';
            searchable += comment;
            if (!containsQuery(searchable, query)) continue;

            pageLines[batchCount] = line;
            repairIds[batchCount] = repairId;
            recordStarts[batchCount] = recordStart;
            ++batchCount;
        }

        if (batchCount == 0U) continue;

        bool closed[MaxListPageSize];
        String closedAt[MaxListPageSize];
        if (!resolveRepairPageStatuses(repairIds, batchCount, closed, closedAt))
        {
            file.close();
            return false;
        }

        for (uint8_t index = 0U; index < batchCount; ++index)
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

            if (!first) json += ',';
            first = false;
            json += decorated;
            ++count;

            if (count >= limit)
            {
                if (index + 1U < batchCount &&
                    !file.seek(recordStarts[index + 1U]))
                {
                    file.close();
                    return false;
                }
                pageFull = true;
                break;
            }
        }
    }

    finishSearchPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}
}
