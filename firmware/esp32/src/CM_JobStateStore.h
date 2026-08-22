#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class JobDeliveryState : uint8_t
{
    Created,
    Delivering,
    Accepted,
    Rejected,
    TimedOut,
    Cancelled
};

enum class JobExecutionState : uint8_t
{
    WaitingDelivery,
    WaitingPhysicalStart,
    Running,
    ProgramCompleted,
    Fault,
    ClosedAfterReview
};

struct JobRuntimeState
{
    uint32_t jobId;
    uint32_t sessionId;
    JobDeliveryState deliveryState;
    JobExecutionState executionState;
    uint32_t lastRunId;
    uint16_t completedRuns;
    uint32_t updatedUptimeMs;

    JobRuntimeState();
};

class JobStateStore
{
public:
    explicit JobStateStore(fs::FS& fileSystem);

    bool begin();
    bool isReady() const;

    bool create(uint32_t jobId, uint32_t sessionId, uint32_t nowMs);
    bool load(uint32_t sessionId, JobRuntimeState& state) const;

    // Finds the highest valid persisted session. Any malformed, temporary or
    // unexpected regular file fails the scan closed. found=false is valid only
    // when the state directory contains no persisted session.
    bool loadLatest(JobRuntimeState& state, bool& found) const;

    bool updateDelivery(uint32_t sessionId,
                        JobDeliveryState deliveryState,
                        uint32_t nowMs);
    bool updateExecution(uint32_t sessionId,
                         JobExecutionState executionState,
                         uint32_t runId,
                         uint16_t completedRuns,
                         uint32_t nowMs);

    // CREATED + WAITING_DELIVERY + zero run evidence is the only durable state
    // that proves local preparation has not crossed the UART delivery boundary.
    // It may remain as immutable audit evidence after a pre-delivery failure.
    static bool isLocalPreparation(const JobRuntimeState& state);

    // Reconciles the narrow lost-JOB_ACK case where delivery was persisted as
    // TIMED_OUT but a later CRC-valid RUN_STARTED proves that Arduino accepted
    // and physically started that exact persisted session. No other timeout
    // state is promoted and no physical action is triggered here.
    bool confirmStartedAfterDeliveryTimeout(uint32_t sessionId,
                                            uint32_t runId,
                                            uint32_t nowMs);

    // Persists an operator-confirmed closure after physical inspection.
    // It never queues, resumes, or controls the machine.
    bool closeAfterManualReview(uint32_t sessionId, uint32_t nowMs);

    // Persists a cancellation only after Arduino has positively confirmed that
    // no remote job remains. This is valid only before any physical-run evidence.
    bool closeAfterRemoteCancel(uint32_t sessionId, uint32_t nowMs);

    // Hides an already inactive job from the active runtime view while keeping
    // its immutable snapshot and historical records. Only terminal delivery or
    // a completed program may be dismissed; accepted/running jobs are refused.
    bool dismissInactive(uint32_t sessionId, uint32_t nowMs);

private:
    static constexpr const char* RootDirectory = "/data/winding-jobs";
    static constexpr const char* StateDirectory = "/data/winding-jobs/state";

    fs::FS& m_fileSystem;
    bool m_ready;

    bool ensureDirectories();
    String statePath(uint32_t sessionId) const;
    String tempPath(uint32_t sessionId) const;
    String backupPath(uint32_t sessionId) const;
    bool writeAtomic(const JobRuntimeState& state);
    bool serialize(const JobRuntimeState& state, String& output) const;
    bool parse(const String& input, JobRuntimeState& state) const;

    static const char* deliveryName(JobDeliveryState state);
    static const char* executionName(JobExecutionState state);
    static bool parseDelivery(const String& value, JobDeliveryState& state);
    static bool parseExecution(const String& value, JobExecutionState& state);
    static bool findUnsigned(const String& input,
                             const char* key,
                             uint32_t& value);
    static bool findString(const String& input,
                           const char* key,
                           String& value);
    static bool validTransition(JobDeliveryState from,
                                JobDeliveryState to);
    static bool validTransition(JobExecutionState from,
                                JobExecutionState to);
};
}
