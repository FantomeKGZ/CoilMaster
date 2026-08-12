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

        uint32_t repairId = 0UL;
        uint32_t lineClientId = 0UL;
        if (!findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned(line, "client_id", lineClientId) || lineClientId == 0UL)
        {
            file.close();
            return false;
        }
        if (clientId > 0UL && lineClientId != clientId) continue;

        bool closed = false;
        String closedAt;
        if (!repairClosed(repairId, closed, closedAt))
        {
            file.close();
            return false;
        }

        String decorated = line;
        decorated.remove(decorated.length() - 1U);
        decorated += F(",\"current_status\":\"");
        decorated += closed ? F("CLOSED") : F("OPEN");
        decorated += F("\",\"closed_at\":");
        if (closed)
        {
            decorated += '"';
            decorated += jsonEscape(closedAt);
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
    }

    finishPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}
}
