#include "CM_JobStateStore.h"

namespace CM
{
JobRuntimeState::JobRuntimeState()
    : jobId(0UL),
      sessionId(0UL),
      deliveryState(JobDeliveryState::Created),
      executionState(JobExecutionState::WaitingDelivery),
      lastRunId(0UL),
      completedRuns(0U),
      updatedUptimeMs(0UL)
{
}

JobStateStore::JobStateStore(fs::FS& fileSystem)
    : m_fileSystem(fileSystem), m_ready(false)
{
}

bool JobStateStore::begin()
{
    m_ready = ensureDirectories();
    return m_ready;
}

bool JobStateStore::isReady() const
{
    if (!m_ready) return false;
    File directory = m_fileSystem.open(StateDirectory, FILE_READ);
    if (!directory) return false;
    const bool ready = directory.isDirectory();
    directory.close();
    return ready;
}

bool JobStateStore::create(uint32_t jobId,
                           uint32_t sessionId,
                           uint32_t nowMs)
{
    if (!isReady() || jobId == 0UL || sessionId == 0UL ||
        m_fileSystem.exists(statePath(sessionId)))
    {
        return false;
    }

    JobRuntimeState state;
    state.jobId = jobId;
    state.sessionId = sessionId;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}

bool JobStateStore::load(uint32_t sessionId, JobRuntimeState& state) const
{
    state = JobRuntimeState();
    if (!isReady() || sessionId == 0UL) return false;

    File file = m_fileSystem.open(statePath(sessionId), FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    if (file.size() == 0U || file.size() >= 320U)
    {
        file.close();
        return false;
    }
    const String input = file.readString();
    file.close();

    return parse(input, state) && state.sessionId == sessionId;
}

bool JobStateStore::loadLatest(JobRuntimeState& state, bool& found) const
{
    state = JobRuntimeState();
    found = false;
    if (!isReady()) return false;

    File directory = m_fileSystem.open(StateDirectory);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }

    uint32_t highestSessionId = 0UL;
    File entry = directory.openNextFile();
    while (entry)
    {
        if (!entry.isDirectory())
        {
            const String name = entry.name();
            if (name.endsWith(F(".json")))
            {
                if (entry.size() == 0U || entry.size() >= 320U)
                {
                    entry.close();
                    directory.close();
                    return false;
                }

                const String input = entry.readString();
                JobRuntimeState candidate;
                if (!parse(input, candidate))
                {
                    entry.close();
                    directory.close();
                    return false;
                }

                const String expectedName = String(F("session-")) +
                                            candidate.sessionId +
                                            F(".json");
                const int separator = name.lastIndexOf('/');
                const String baseName = separator >= 0
                    ? name.substring(separator + 1)
                    : name;
                if (baseName != expectedName)
                {
                    entry.close();
                    directory.close();
                    return false;
                }

                if (candidate.sessionId > highestSessionId)
                {
                    highestSessionId = candidate.sessionId;
                    state = candidate;
                    found = true;
                }
            }
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return true;
}

bool JobStateStore::updateDelivery(uint32_t sessionId,
                                   JobDeliveryState deliveryState,
                                   uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state) ||
        !validTransition(state.deliveryState, deliveryState))
    {
        return false;
    }

    state.deliveryState = deliveryState;
    if (deliveryState == JobDeliveryState::Accepted &&
        state.executionState == JobExecutionState::WaitingDelivery)
    {
        state.executionState = JobExecutionState::WaitingPhysicalStart;
    }
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}

bool JobStateStore::updateExecution(uint32_t sessionId,
                                    JobExecutionState executionState,
                                    uint32_t runId,
                                    uint16_t completedRuns,
                                    uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state) ||
        !validTransition(state.executionState, executionState))
    {
        return false;
    }

    if (executionState == JobExecutionState::Running)
    {
        if (state.deliveryState != JobDeliveryState::Accepted ||
            runId == 0UL || runId <= state.lastRunId ||
            completedRuns != state.completedRuns)
        {
            return false;
        }
        state.lastRunId = runId;
    }
    else if (executionState == JobExecutionState::ProgramCompleted)
    {
        if (runId == 0UL || runId != state.lastRunId ||
            completedRuns == 0U ||
            state.completedRuns == 0xFFFFU ||
            completedRuns != static_cast<uint16_t>(state.completedRuns + 1U))
        {
            return false;
        }
        state.completedRuns = completedRuns;
    }

    state.executionState = executionState;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}

bool JobStateStore::closeAfterManualReview(uint32_t sessionId,
                                           uint32_t nowMs)
{
    JobRuntimeState state;
    if (!load(sessionId, state)) return false;

    if (state.executionState == JobExecutionState::ClosedAfterReview)
        return true;

    const bool reviewRequired =
        state.executionState == JobExecutionState::Running ||
        state.executionState == JobExecutionState::Fault ||
        state.deliveryState == JobDeliveryState::Delivering ||
        (state.deliveryState == JobDeliveryState::Accepted &&
         state.executionState == JobExecutionState::WaitingPhysicalStart);
    if (!reviewRequired) return false;

    state.executionState = JobExecutionState::ClosedAfterReview;
    state.updatedUptimeMs = nowMs;
    return writeAtomic(state);
}

bool JobStateStore::ensureDirectories()
{
    if (!m_fileSystem.exists("/data") && !m_fileSystem.mkdir("/data"))
        return false;
    if (!m_fileSystem.exists(RootDirectory) &&
        !m_fileSystem.mkdir(RootDirectory))
        return false;
    if (!m_fileSystem.exists(StateDirectory) &&
        !m_fileSystem.mkdir(StateDirectory))
        return false;
    return true;
}

String JobStateStore::statePath(uint32_t sessionId) const
{
    return String(StateDirectory) + F("/session-") + sessionId + F(".json");
}

String JobStateStore::tempPath(uint32_t sessionId) const
{
    return String(StateDirectory) + F("/session-") + sessionId + F(".tmp");
}

bool JobStateStore::writeAtomic(const JobRuntimeState& state)
{
    if (!isReady() || state.jobId == 0UL || state.sessionId == 0UL)
        return false;

    String output;
    if (!serialize(state, output)) return false;

    const String target = statePath(state.sessionId);
    const String temp = tempPath(state.sessionId);
    if (m_fileSystem.exists(temp) && !m_fileSystem.remove(temp)) return false;

    File file = m_fileSystem.open(temp, FILE_WRITE);
    if (!file) return false;
    const size_t written = file.print(output);
    file.flush();
    file.close();
    if (written != output.length())
    {
        m_fileSystem.remove(temp);
        return false;
    }

    File verify = m_fileSystem.open(temp, FILE_READ);
    if (!verify)
    {
        m_fileSystem.remove(temp);
        return false;
    }
    const String verifiedText = verify.readString();
    verify.close();
    JobRuntimeState verifiedState;
    if (!parse(verifiedText, verifiedState) ||
        verifiedState.jobId != state.jobId ||
        verifiedState.sessionId != state.sessionId ||
        verifiedState.deliveryState != state.deliveryState ||
        verifiedState.executionState != state.executionState ||
        verifiedState.lastRunId != state.lastRunId ||
        verifiedState.completedRuns != state.completedRuns ||
        verifiedState.updatedUptimeMs != state.updatedUptimeMs)
    {
        m_fileSystem.remove(temp);
        return false;
    }

    if (m_fileSystem.exists(target) && !m_fileSystem.remove(target))
    {
        m_fileSystem.remove(temp);
        return false;
    }
    if (!m_fileSystem.rename(temp, target))
    {
        m_fileSystem.remove(temp);
        return false;
    }
    return true;
}

bool JobStateStore::serialize(const JobRuntimeState& state,
                              String& output) const
{
    output = F("{\"schema_version\":1,\"job_id\":");
    output += state.jobId;
    output += F(",\"session_id\":");
    output += state.sessionId;
    output += F(",\"delivery_state\":\"");
    output += deliveryName(state.deliveryState);
    output += F("\",\"execution_state\":\"");
    output += executionName(state.executionState);
    output += F("\",\"last_run_id\":");
    output += state.lastRunId;
    output += F(",\"completed_runs\":");
    output += state.completedRuns;
    output += F(",\"updated_uptime_ms\":");
    output += state.updatedUptimeMs;
    output += F("}\n");
    return output.length() < 320U;
}

bool JobStateStore::parse(const String& input, JobRuntimeState& state) const
{
    state = JobRuntimeState();
    if (!input.startsWith(F("{\"schema_version\":1,")) ||
        !input.endsWith(F("}\n")) || input.length() >= 320U)
    {
        return false;
    }

    uint32_t schemaVersion = 0UL;
    uint32_t completedRuns = 0UL;
    String delivery;
    String execution;
    if (!findUnsigned(input, "schema_version", schemaVersion) ||
        schemaVersion != 1UL ||
        !findUnsigned(input, "job_id", state.jobId) || state.jobId == 0UL ||
        !findUnsigned(input, "session_id", state.sessionId) || state.sessionId == 0UL ||
        !findString(input, "delivery_state", delivery) ||
        !findString(input, "execution_state", execution) ||
        !findUnsigned(input, "last_run_id", state.lastRunId) ||
        !findUnsigned(input, "completed_runs", completedRuns) ||
        completedRuns > 0xFFFFUL ||
        !findUnsigned(input, "updated_uptime_ms", state.updatedUptimeMs) ||
        !parseDelivery(delivery, state.deliveryState) ||
        !parseExecution(execution, state.executionState))
    {
        return false;
    }

    state.completedRuns = static_cast<uint16_t>(completedRuns);

    if (state.executionState == JobExecutionState::WaitingDelivery)
    {
        const bool deliveryConsistent =
            state.deliveryState == JobDeliveryState::Created ||
            state.deliveryState == JobDeliveryState::Delivering ||
            state.deliveryState == JobDeliveryState::Rejected ||
            state.deliveryState == JobDeliveryState::TimedOut ||
            state.deliveryState == JobDeliveryState::Cancelled;
        return deliveryConsistent && state.lastRunId == 0UL &&
               state.completedRuns == 0U;
    }
    if (state.executionState == JobExecutionState::WaitingPhysicalStart)
    {
        return state.deliveryState == JobDeliveryState::Accepted &&
               state.lastRunId == 0UL && state.completedRuns == 0U;
    }
    if (state.executionState == JobExecutionState::Running)
    {
        return state.deliveryState == JobDeliveryState::Accepted &&
               state.lastRunId != 0UL;
    }
    if (state.executionState == JobExecutionState::ProgramCompleted)
    {
        return state.deliveryState == JobDeliveryState::Accepted &&
               state.lastRunId != 0UL && state.completedRuns != 0U;
    }
    if (state.executionState == JobExecutionState::ClosedAfterReview)
    {
        return true;
    }
    return state.executionState == JobExecutionState::Fault;
}

const char* JobStateStore::deliveryName(JobDeliveryState state)
{
    switch (state)
    {
        case JobDeliveryState::Created: return "CREATED";
        case JobDeliveryState::Delivering: return "DELIVERING";
        case JobDeliveryState::Accepted: return "ACCEPTED";
        case JobDeliveryState::Rejected: return "REJECTED";
        case JobDeliveryState::TimedOut: return "TIMED_OUT";
        case JobDeliveryState::Cancelled: return "CANCELLED";
        default: return "CREATED";
    }
}

const char* JobStateStore::executionName(JobExecutionState state)
{
    switch (state)
    {
        case JobExecutionState::WaitingDelivery: return "WAITING_DELIVERY";
        case JobExecutionState::WaitingPhysicalStart: return "WAITING_PHYSICAL_START";
        case JobExecutionState::Running: return "RUNNING";
        case JobExecutionState::ProgramCompleted: return "PROGRAM_COMPLETED";
        case JobExecutionState::Fault: return "FAULT";
        case JobExecutionState::ClosedAfterReview: return "CLOSED_AFTER_REVIEW";
        default: return "WAITING_DELIVERY";
    }
}

bool JobStateStore::parseDelivery(const String& value,
                                  JobDeliveryState& state)
{
    if (value == "CREATED") state = JobDeliveryState::Created;
    else if (value == "DELIVERING") state = JobDeliveryState::Delivering;
    else if (value == "ACCEPTED") state = JobDeliveryState::Accepted;
    else if (value == "REJECTED") state = JobDeliveryState::Rejected;
    else if (value == "TIMED_OUT") state = JobDeliveryState::TimedOut;
    else if (value == "CANCELLED") state = JobDeliveryState::Cancelled;
    else return false;
    return true;
}

bool JobStateStore::parseExecution(const String& value,
                                   JobExecutionState& state)
{
    if (value == "WAITING_DELIVERY") state = JobExecutionState::WaitingDelivery;
    else if (value == "WAITING_PHYSICAL_START") state = JobExecutionState::WaitingPhysicalStart;
    else if (value == "RUNNING") state = JobExecutionState::Running;
    else if (value == "PROGRAM_COMPLETED") state = JobExecutionState::ProgramCompleted;
    else if (value == "FAULT") state = JobExecutionState::Fault;
    else if (value == "CLOSED_AFTER_REVIEW") state = JobExecutionState::ClosedAfterReview;
    else return false;
    return true;
}

bool JobStateStore::findUnsigned(const String& input,
                                 const char* key,
                                 uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = input.indexOf(marker);
    if (position < 0 || input.indexOf(marker, position + marker.length()) >= 0)
        return false;

    int cursor = position + marker.length();
    if (cursor >= input.length() || !isDigit(input[cursor])) return false;
    if (input[cursor] == '0' && cursor + 1 < input.length() &&
        isDigit(input[cursor + 1])) return false;

    uint32_t parsed = 0UL;
    while (cursor < input.length() && isDigit(input[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(input[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }

    if (cursor >= input.length() ||
        (input[cursor] != ',' && input[cursor] != '}'))
    {
        return false;
    }

    value = parsed;
    return true;
}

bool JobStateStore::findString(const String& input,
                               const char* key,
                               String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int position = input.indexOf(marker);
    if (position < 0 || input.indexOf(marker, position + marker.length()) >= 0)
        return false;

    int cursor = position + marker.length();
    while (cursor < input.length())
    {
        const char ch = input[cursor++];
        if (ch == '"')
        {
            return cursor < input.length() &&
                   (input[cursor] == ',' || input[cursor] == '}') &&
                   value.length() > 0U;
        }
        if (ch == '\\' || static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool JobStateStore::validTransition(JobDeliveryState from,
                                    JobDeliveryState to)
{
    if (from == to) return true;
    if (from == JobDeliveryState::Created)
        return to == JobDeliveryState::Delivering;
    if (from == JobDeliveryState::Delivering)
        return to == JobDeliveryState::Accepted ||
               to == JobDeliveryState::Rejected ||
               to == JobDeliveryState::TimedOut ||
               to == JobDeliveryState::Cancelled;
    return false;
}

bool JobStateStore::validTransition(JobExecutionState from,
                                    JobExecutionState to)
{
    if (from == JobExecutionState::ClosedAfterReview)
        return to == JobExecutionState::ClosedAfterReview;
    if (from == to) return true;
    if (to == JobExecutionState::Fault) return true;
    if (from == JobExecutionState::WaitingDelivery)
        return to == JobExecutionState::WaitingPhysicalStart;
    if (from == JobExecutionState::WaitingPhysicalStart ||
        from == JobExecutionState::ProgramCompleted)
        return to == JobExecutionState::Running;
    if (from == JobExecutionState::Running)
        return to == JobExecutionState::ProgramCompleted;
    return false;
}
}
