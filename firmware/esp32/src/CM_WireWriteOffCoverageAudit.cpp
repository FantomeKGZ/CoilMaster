#include "CM_WireWriteOffCoverageAudit.h"
#include "CM_WarehouseMovementIntegrityAudit.h"
#include "CM_WindingJournalQuery.h"
#include <string.h>

namespace CM
{
namespace
{
constexpr const char* SelectionDirectory = "/data/winding-jobs/spool-selection";
constexpr const char* MovementsPath = "/data/warehouse/movements.ndjson";

enum class ReadOnlyCatalogCheck : uint8_t
{
    Ready,
    Missing,
    StorageUnavailable,
    IntegrityFailed
};

struct SelectionIdentity
{
    uint32_t sessionId;
    uint32_t repairId;
    SelectionIdentity() : sessionId(0UL), repairId(0UL) {}
};

String baseNameOf(const String& path)
{
    const int slash = path.lastIndexOf('/');
    return slash >= 0 ? path.substring(slash + 1) : path;
}

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

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"')
            return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
        if (ch == '\\' || static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool canonicalSelectionName(const String& path, uint32_t& sessionId)
{
    sessionId = 0UL;
    const String name = baseNameOf(path);
    if (!name.startsWith(F("session-")) || !name.endsWith(F(".json"))) return false;
    const String digits = name.substring(8U, name.length() - 5U);
    if (digits.length() == 0U || (digits.length() > 1U && digits[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < digits.length(); ++i)
    {
        if (!isDigit(digits[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(digits[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed == 0UL) return false;
    sessionId = parsed;
    return true;
}

bool parseSelection(const String& input, SelectionIdentity& identity)
{
    identity = SelectionIdentity();
    if (!input.startsWith(F("{\"schema_version\":1,")) ||
        !input.endsWith(F("}\n")) || input.length() >= 512U)
        return false;

    uint32_t schema = 0UL, jobId = 0UL, motorId = 0UL, spoolId = 0UL;
    uint32_t diameter = 0UL, weight = 0UL;
    String wireType;
    if (!findUnsigned(input, "schema_version", schema) || schema != 1UL ||
        !findUnsigned(input, "job_id", jobId) || jobId == 0UL ||
        !findUnsigned(input, "session_id", identity.sessionId) || identity.sessionId == 0UL ||
        !findUnsigned(input, "repair_id", identity.repairId) || identity.repairId == 0UL ||
        !findUnsigned(input, "motor_id", motorId) || motorId == 0UL ||
        !findUnsigned(input, "spool_id", spoolId) || spoolId == 0UL ||
        !findUnsigned(input, "diameter_hundredths_mm", diameter) ||
        diameter == 0UL || diameter > 0xFFFFUL ||
        !findUnsigned(input, "weight_at_selection_g", weight) || weight == 0UL ||
        !findString(input, "wire_type", wireType) ||
        (wireType != "CU" && wireType != "AL"))
        return false;

    const String marker = F("\"automatic_writeoff_allowed\":false");
    const int markerPos = input.indexOf(marker);
    return markerPos >= 0 &&
           input.indexOf(marker, markerPos + marker.length()) < 0;
}

ReadOnlyCatalogCheck auditSelectionDirectory(fs::FS& storage)
{
    if (!storage.exists(SelectionDirectory)) return ReadOnlyCatalogCheck::Missing;
    File directory = storage.open(SelectionDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return ReadOnlyCatalogCheck::StorageUnavailable;
    }

    File entry = directory.openNextFile();
    while (entry)
    {
        if (entry.isDirectory() || entry.size() == 0U || entry.size() >= 512U)
        {
            entry.close();
            directory.close();
            return ReadOnlyCatalogCheck::IntegrityFailed;
        }
        uint32_t fileSessionId = 0UL;
        if (!canonicalSelectionName(entry.name(), fileSessionId))
        {
            entry.close();
            directory.close();
            return ReadOnlyCatalogCheck::IntegrityFailed;
        }
        const String content = entry.readString();
        entry.close();
        SelectionIdentity identity;
        if (!parseSelection(content, identity) || identity.sessionId != fileSessionId)
        {
            directory.close();
            return ReadOnlyCatalogCheck::IntegrityFailed;
        }
        entry = directory.openNextFile();
    }
    directory.close();
    return ReadOnlyCatalogCheck::Ready;
}

bool loadSelectionReadOnly(fs::FS& storage,
                           uint32_t sessionId,
                           SelectionIdentity& identity,
                           bool& found)
{
    identity = SelectionIdentity();
    found = false;
    String path = F("/data/winding-jobs/spool-selection/session-");
    path += sessionId;
    path += F(".json");
    if (!storage.exists(path)) return true;

    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory() || file.size() == 0U || file.size() >= 512U)
    {
        if (file) file.close();
        return false;
    }
    const String content = file.readString();
    file.close();
    if (!parseSelection(content, identity) || identity.sessionId != sessionId) return false;
    found = true;
    return true;
}

bool confirmedWriteOffExists(fs::FS& storage,
                             uint32_t sessionId,
                             uint32_t runId,
                             bool& found)
{
    found = false;
    if (!storage.exists(MovementsPath)) return true;
    File file = storage.open(MovementsPath, FILE_READ);
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

bool pageRecordAt(const String& page, int& cursor, String& record)
{
    record = String();
    while (cursor < page.length() && (page[cursor] == ',' || page[cursor] == ' ')) ++cursor;
    if (cursor >= page.length() || page[cursor] != '{') return false;
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

    const ReadOnlyCatalogCheck selectionCatalog = auditSelectionDirectory(storage);
    if (selectionCatalog == ReadOnlyCatalogCheck::StorageUnavailable)
        return WireWriteOffCoverageCheck::StorageUnavailable;
    if (selectionCatalog == ReadOnlyCatalogCheck::IntegrityFailed)
        return WireWriteOffCoverageCheck::IntegrityFailed;

    if (storage.exists(MovementsPath) && !WarehouseMovementIntegrityAudit::check(storage))
    {
        File probe = storage.open(MovementsPath, FILE_READ);
        if (!probe) return WireWriteOffCoverageCheck::StorageUnavailable;
        probe.close();
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
            history.appendHistoryJson(0UL, repairId, cursor, 100U,
                                      page, count, nextCursor, hasMore);
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

            uint32_t sessionId = 0UL, runId = 0UL;
            if (!findUnsigned(record, "session_id", sessionId) || sessionId == 0UL ||
                !findUnsigned(record, "run_id", runId) || runId == 0UL)
                return WireWriteOffCoverageCheck::IntegrityFailed;

            if (selectionCatalog == ReadOnlyCatalogCheck::Missing) continue;

            SelectionIdentity selection;
            bool selectionFound = false;
            if (!loadSelectionReadOnly(storage, sessionId, selection, selectionFound))
                return WireWriteOffCoverageCheck::IntegrityFailed;
            if (!selectionFound) continue; // Legacy linked session.
            if (selection.repairId != repairId)
                return WireWriteOffCoverageCheck::IntegrityFailed;

            bool writeOffFound = false;
            if (!confirmedWriteOffExists(storage, sessionId, runId, writeOffFound))
            {
                File probe = storage.open(MovementsPath, FILE_READ);
                if (!probe) return WireWriteOffCoverageCheck::StorageUnavailable;
                probe.close();
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
