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
    Fault
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

    bool updateDelivery(uint32_t sessionId,
                        JobDeliveryState deliveryState,
                        uint32_t nowMs);
    bool updateExecution(uint32_t sessionId,
                         JobExecutionState executionState,
                         uint32_t runId,
                         uint16_t completedRuns,
                         uint32_t nowMs);

private:
    static constexpr const char* RootDirectory = "/data/winding-jobs";
    static constexpr const char* StateDirectory = "/data/winding-jobs/state";

    fs::FS& m_fileSystem;
    bool m_ready;

    bool ensureDirectories();
    String statePath(uint32_t sessionId) const;
    String tempPath(uint32_t sessionId) const;
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
