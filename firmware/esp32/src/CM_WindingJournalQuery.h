#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class WindingJournalQueryResult : uint8_t
{
    Ok,
    InvalidFilter,
    StorageUnavailable,
    ReadFailed
};

class WindingJournalQuery
{
public:
    explicit WindingJournalQuery(fs::FS& storage);

    bool begin();
    bool isReady() const;

    // Exactly one non-zero filter must be supplied. Only validated schema 2
    // records are returned. limit is clamped to 1..100.
    WindingJournalQueryResult appendHistoryJson(uint32_t sessionId,
                                                uint32_t repairId,
                                                uint16_t limit,
                                                String& json,
                                                uint16_t& count) const;

    // Cursor is the number of already-consumed matching validated records.
    // nextCursor advances by count. hasMore is true only when another matching
    // validated record exists after the returned page.
    WindingJournalQueryResult appendHistoryJson(uint32_t sessionId,
                                                uint32_t repairId,
                                                uint32_t cursor,
                                                uint16_t limit,
                                                String& json,
                                                uint16_t& count,
                                                uint32_t& nextCursor,
                                                bool& hasMore) const;

private:
    static constexpr const char* JournalPath = "/data/winding-runs/events.ndjson";

    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);
    static bool fieldIsNull(const String& line, const char* key);
    static bool isValidLegacySchema1Record(const String& line);
    static bool isValidSchema2Record(const String& line,
                                     uint32_t& sessionId,
                                     bool& linked,
                                     uint32_t& repairId);

    fs::FS& m_storage;
    bool m_ready;
};
}
