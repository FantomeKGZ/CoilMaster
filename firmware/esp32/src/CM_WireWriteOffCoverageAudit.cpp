#include "CM_WireWriteOffCoverageAudit.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_WarehouseMovementIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"

namespace CM
{
namespace
{
bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool confirmedWriteOffExists(fs::FS& storage,
                             uint32_t sessionId,
                             uint32_t runId,
                             bool& found)
{
    found = false;
    constexpr const char* Path = "/data/warehouse/movements.ndjson";
    if (!storage.exists(Path)) return true;
    if (!WarehouseMovementIntegrityAudit::check(storage)) return false;

    File file = storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (line.indexOf(F("\"status\":\"CONFIRMED\"")) < 0) continue;

        const bool hasSession = line.indexOf(F("\"source_session_id\":")) >= 0;
        if (!hasSession) continue;

        uint32_t currentSession = 0UL;
        if (!findUnsigned(line, "source_session_id", currentSession) || currentSession == 0UL)
        {
            file.close();
            return false;
        }
        if (currentSession != sessionId) continue;

        const bool hasRun = line.indexOf(F("\"source_run_id\":")) >= 0;
        if (!hasRun)
        {
            // Legacy session-level provenance covers the whole session.
            found = true;
            continue;
        }

        uint32_t currentRun = 0UL;
        if (!findUnsigned(line, "source_run_id", currentRun) || currentRun == 0UL)
        {
            file.close();
            return false;
        }
        if (currentRun == runId) found = true;
    }

    file.close();
    return true;
}

bool pageRecordAt(const String& page,
                  int& cursor,
                  String& record)
{
    record = String();
    while (cursor < page.length() && (page[cursor] == ',' || page[cursor] == ' ')) ++cursor;
    if (cursor >= page.length()) return false;
    if (page[cursor] != '{') return false;
    const int end = page.indexOf('}', cursor + 1);
    if (end < 0) return false;
    record = page.substring(cursor, end + 1);
    cursor = end + 1;
    return true;
}
}

WireWriteOffCoverageCheck WireWriteOffCoverageAudit::check(fs::FS& storage,
                                                           uint32_t repairId)
{
    if (repairId == 0UL) return WireWriteOffCoverageCheck::IntegrityFailed;

    WindingJournalQuery history(storage);
    if (!history.begin() || !history.isReady())
        return WireWriteOffCoverageCheck::StorageUnavailable;

    constexpr const char* SelectionDirectory = "/data/winding-jobs/spool-selection";
    const bool selectionDirectoryExists = storage.exists(SelectionDirectory);
    JobSpoolSelectionStore selections(storage);
    if (selectionDirectoryExists && !selections.begin())
    {
        File root = storage.open("/", FILE_READ);
        if (!root) return WireWriteOffCoverageCheck::StorageUnavailable;
        root.close();
        return WireWriteOffCoverageCheck::IntegrityFailed;
    }

    uint32_t cursor = 0UL;
    for (;;)
    {
        String page;
        page.reserve(4096U);
        uint16_t count = 0U;
        uint32_t nextCursor = cursor;
        bool hasMore = false;
        const WindingJournalQueryResult result =
            history.appendHistoryJson(0UL,
                                      repairId,
                                      cursor,
                                      100U,
                                      page,
                                      count,
                                      nextCursor,
                                      hasMore);
        if (result == WindingJournalQueryResult::StorageUnavailable)
            return WireWriteOffCoverageCheck::StorageUnavailable;
        if (result != WindingJournalQueryResult::Ok)
            return WireWriteOffCoverageCheck::IntegrityFailed;

        int pageCursor = 0;
        uint16_t parsedCount = 0U;
        while (pageCursor < page.length())
        {
            String record;
            if (!pageRecordAt(page, pageCursor, record))
                return WireWriteOffCoverageCheck::IntegrityFailed;
            ++parsedCount;
            if (record.indexOf(F("\"event\":\"RUN_COMPLETED\"")) < 0) continue;

            uint32_t sessionId = 0UL;
            uint32_t runId = 0UL;
            if (!findUnsigned(record, "session_id", sessionId) || sessionId == 0UL ||
                !findUnsigned(record, "run_id", runId) || runId == 0UL)
            {
                return WireWriteOffCoverageCheck::IntegrityFailed;
            }

            if (!selectionDirectoryExists) continue;

            JobSpoolSelection selection;
            bool selectionFound = false;
            if (!selections.load(sessionId, selection, selectionFound))
            {
                return selections.isReady()
                    ? WireWriteOffCoverageCheck::IntegrityFailed
                    : WireWriteOffCoverageCheck::StorageUnavailable;
            }
            if (!selectionFound) continue; // Legacy linked session.
            if (selection.repairId != repairId)
                return WireWriteOffCoverageCheck::IntegrityFailed;

            bool writeOffFound = false;
            if (!confirmedWriteOffExists(storage, sessionId, runId, writeOffFound))
            {
                File root = storage.open("/", FILE_READ);
                if (!root) return WireWriteOffCoverageCheck::StorageUnavailable;
                root.close();
                return WireWriteOffCoverageCheck::IntegrityFailed;
            }
            if (!writeOffFound)
                return WireWriteOffCoverageCheck::WriteOffRequired;
        }

        if (parsedCount != count)
            return WireWriteOffCoverageCheck::IntegrityFailed;
        if (!hasMore) break;
        if (count == 0U || nextCursor <= cursor)
            return WireWriteOffCoverageCheck::IntegrityFailed;
        cursor = nextCursor;
    }

    return WireWriteOffCoverageCheck::Covered;
}
}
