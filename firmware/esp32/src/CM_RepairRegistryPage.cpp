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
        const String baseLine = file.readStringUntil('\n');
        if (baseLine.length() == 0U) continue;
        uint32_t clientId = 0UL;
        if (!FlatJsonObjectValidator::valid(baseLine) ||
            !findUnsigned(baseLine, "client_id", clientId) || clientId == 0UL)
        {
            file.close();
            return false;
        }

        String effectiveLine = baseLine;
        String revision;
        bool revisionFound = false;
        if (!latestClientRevisionLine(clientId, revision, revisionFound))
        {
            file.close();
            return false;
        }
        if (revisionFound) effectiveLine = revision;

        String normalized;
        if (query.length() > 0U &&
            (!findString(effectiveLine, "phone_normalized", normalized) ||
             normalized.indexOf(query) < 0))
        {
            continue;
        }

        if (!first) json += ',';
        first = false;
        json += effectiveLine;
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
        uint32_t motorId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL)
        {
            file.close();
            return false;
        }

        String effectiveLine = line;
        String revision;
        bool revisionFound = false;
        if (!latestMotorRevisionLine(motorId, revision, revisionFound))
        {
            file.close();
            return false;
        }
        if (revisionFound) effectiveLine = revision;

        if (query.length() > 0U)
        {
            String searchable;
            String field;
            if (findString(effectiveLine, "name", field)) searchable += field + ' ';
            if (findString(effectiveLine, "model", field)) searchable += field + ' ';
            if (findString(effectiveLine, "manufacturer", field)) searchable += field + ' ';
            if (findString(effectiveLine, "tags", field)) searchable += field + ' ';
            if (findString(effectiveLine, "coil_program", field)) searchable += field + ' ';

            uint32_t numeric = 0UL;
            if (findUnsigned(effectiveLine, "phases", numeric))
            {
                searchable += String(numeric);
                searchable += F(" фазы фаз ");
            }
            if (findUnsigned(effectiveLine, "slot_count", numeric))
            {
                searchable += String(numeric);
                searchable += F(" пазы пазов ");
            }
            if (findUnsigned(effectiveLine, "repeat_target", numeric))
            {
                searchable += String(numeric);
                searchable += F(" повторы повторов ");
            }
            if (findUnsigned(effectiveLine, "pole_count", numeric))
            {
                searchable += String(numeric);
                searchable += F(" полюсы полюсов ");
            }

            searchable.toLowerCase();
            if (searchable.indexOf(query) < 0) continue;
        }

        if (!first) json += ',';
        first = false;
        json += effectiveLine;
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
                                           uint32_t motorId,
                                           const String& statusFilter,
                                           uint32_t cursor,
                                           uint8_t limit,
                                           uint16_t& count,
                                           uint32_t& nextCursor,
                                           bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || limit == 0U || limit > MaxListPageSize ||
        (statusFilter.length() > 0U &&
         statusFilter != "OPEN" && statusFilter != "CLOSED"))
    {
        return false;
    }
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
            uint32_t lineClientId = 0UL;
            uint32_t lineMotorId = 0UL;
            if (!findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
                !findUnsigned(line, "client_id", lineClientId) || lineClientId == 0UL ||
                !findUnsigned(line, "motor_id", lineMotorId) || lineMotorId == 0UL)
            {
                file.close();
                return false;
            }
            if (clientId > 0UL && lineClientId != clientId) continue;
            if (motorId > 0UL && lineMotorId != motorId) continue;

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
            const bool statusMatches =
                statusFilter.length() == 0U ||
                (statusFilter == "CLOSED" && closed[index]) ||
                (statusFilter == "OPEN" && !closed[index]);
            if (!statusMatches) continue;

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

    finishPage(file, fileSize, nextCursor, hasMore);
    file.close();
    return true;
}
}
