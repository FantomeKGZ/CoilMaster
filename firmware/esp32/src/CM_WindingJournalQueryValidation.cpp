#include "CM_WindingJournalQuery.h"

namespace CM
{
bool WindingJournalQuery::isValidRecord(const String& line)
{
    if (line.length() == 0U) return false;

    uint32_t schemaVersion = 0UL;
    if (!findUnsigned(line, "schema_version", schemaVersion) ||
        (schemaVersion != 1UL && schemaVersion != 2UL))
    {
        return false;
    }

    if (schemaVersion == 1UL)
        return isValidLegacySchema1Record(line);

    uint32_t sessionId = 0UL;
    uint32_t repairId = 0UL;
    bool linked = false;
    return isValidSchema2Record(line, sessionId, linked, repairId);
}

WindingJournalQueryResult WindingJournalQuery::validateAll() const
{
    uint32_t ignoredRecordCount = 0UL;
    return validateAll(ignoredRecordCount);
}

WindingJournalQueryResult WindingJournalQuery::validateAll(uint32_t& recordCount) const
{
    recordCount = 0UL;
    if (!isReady()) return WindingJournalQueryResult::StorageUnavailable;
    if (!m_storage.exists(JournalPath)) return WindingJournalQueryResult::Ok;

    File file = m_storage.open(JournalPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return WindingJournalQueryResult::ReadFailed;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (recordCount == 0xFFFFFFFFUL || !isValidRecord(line))
        {
            file.close();
            return WindingJournalQueryResult::ReadFailed;
        }
        ++recordCount;
    }

    file.close();
    return WindingJournalQueryResult::Ok;
}
}
