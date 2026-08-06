#ifndef CM_WINDING_JOURNAL_HEALTH_H
#define CM_WINDING_JOURNAL_HEALTH_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct WindingJournalHealthReport
{
    uint32_t recordCount;
    uint32_t malformedRecordCount;
    uint32_t startedCount;
    uint32_t completedCount;
    uint32_t activeRunCount;
    bool readable;
    bool sequenceConsistent;

    WindingJournalHealthReport()
        : recordCount(0UL), malformedRecordCount(0UL),
          startedCount(0UL), completedCount(0UL), activeRunCount(0UL),
          readable(false), sequenceConsistent(true) {}

    bool healthy() const
    {
        return readable && malformedRecordCount == 0UL &&
               sequenceConsistent && activeRunCount <= 1UL;
    }
};

class WindingJournalHealth
{
public:
    explicit WindingJournalHealth(fs::FS& fileSystem);

    bool inspect(WindingJournalHealthReport& report) const;

private:
    static constexpr const char* JournalPath =
        "/data/winding-runs/events.ndjson";

    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);

    fs::FS& m_fileSystem;
};
}

#endif // CM_WINDING_JOURNAL_HEALTH_H
