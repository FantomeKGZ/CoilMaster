#ifndef CM_WINDING_JOURNAL_H
#define CM_WINDING_JOURNAL_H

#include <Arduino.h>
#include <FS.h>

#include "CM_UartEventReceiver.h"

namespace CM
{
enum class JournalSaveResult : uint8_t
{
    Saved = 0U,
    Duplicate,
    StorageUnavailable,
    WriteFailed,
    InvalidTransition
};

struct WindingEventContext
{
    uint32_t jobId;
    bool linked;
    uint32_t repairId;
    uint32_t motorId;

    WindingEventContext()
        : jobId(0UL), linked(false), repairId(0UL), motorId(0UL) {}

    bool isValid() const
    {
        return jobId != 0UL &&
               ((!linked && repairId == 0UL && motorId == 0UL) ||
                (linked && repairId != 0UL && motorId != 0UL));
    }
};

struct WindingSessionState
{
    uint32_t sessionId;
    uint32_t activeRunId;
    uint32_t highestRunId;
    uint16_t completedRuns;
    bool activeRunFound;
    bool journalConsistent;

    WindingSessionState()
        : sessionId(0UL), activeRunId(0UL), highestRunId(0UL),
          completedRuns(0U), activeRunFound(false), journalConsistent(false) {}
};

class WindingJournal
{
public:
    explicit WindingJournal(fs::FS& fileSystem);

    bool begin();
    bool isReady() const;

    // Resolves job and repair context from the immutable session snapshot.
    JournalSaveResult save(const RemoteWindingEvent& event);

    // Writes a record only with already validated immutable context.
    JournalSaveResult save(const RemoteWindingEvent& event,
                           const WindingEventContext& context);

    bool loadSessionState(uint32_t sessionId,
                          WindingSessionState& state) const;

private:
    static constexpr const char* DirectoryPath = "/data/winding-runs";
    static constexpr const char* JournalPath = "/data/winding-runs/events.ndjson";

    bool ensureDirectories();
    bool validateJournalStructure() const;
    bool validateJournalSessionContexts() const;
    bool sessionContextMatches(uint32_t sessionId,
                               const WindingEventContext& context) const;
    bool containsRunEvent(uint32_t sessionId,
                          uint32_t runId,
                          RemoteEventType type) const;
    bool hasRunStart(uint32_t sessionId, uint32_t runId) const;
    bool loadSessionCompletedRuns(uint32_t sessionId,
                                  uint16_t& completedRuns) const;
    bool loadActiveRun(uint32_t sessionId,
                       uint32_t& runId,
                       bool& found) const;
    bool loadSessionHighestRunId(uint32_t sessionId,
                                 uint32_t& highestRunId) const;
    bool appendRecord(const RemoteWindingEvent& event,
                      const WindingEventContext& context);
    static bool parseContext(const String& line,
                             WindingEventContext& context);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool fieldIsNull(const String& line, const char* key);
    static const char* eventTypeName(RemoteEventType type);

    fs::FS& m_fileSystem;
    bool m_ready;
};
}

#endif // CM_WINDING_JOURNAL_H
