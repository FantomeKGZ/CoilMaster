#ifndef CM_UART_EVENT_RECEIVER_H
#define CM_UART_EVENT_RECEIVER_H

#include <Arduino.h>

#include "CM_HallCalibrationCompletionAdapter.h"
#include "CM_HallCalibrationDoneProtocol.h"
#include "CM_HallCalibrationRawCollector.h"
#include "CM_HallCalibrationRawProtocol.h"
#include "CM_HardwareControlClient.h"

namespace CM
{
enum class RemoteEventType : uint8_t
{
    None = 0U,
    RunStarted,
    RunCompleted
};

enum class RemoteJobType : uint8_t
{
    Working = 0U,
    Starting
};

struct RemoteWindingEvent
{
    static constexpr uint8_t MaxCoils = 10U;

    RemoteEventType type;
    uint32_t sessionId;
    uint32_t runId;
    uint16_t completedRuns;
    bool localStandalone;
    RemoteJobType jobType;
    uint8_t coilCount;
    uint16_t turns[MaxCoils];

    RemoteWindingEvent()
        : type(RemoteEventType::None), sessionId(0UL), runId(0UL),
          completedRuns(0U), localStandalone(false),
          jobType(RemoteJobType::Working), coilCount(0U), turns()
    {
    }

    bool hasProgram() const
    {
        if (!localStandalone || coilCount == 0U || coilCount > MaxCoils)
            return false;
        for (uint8_t index = 0U; index < coilCount; ++index)
            if (turns[index] == 0U || turns[index] > 9999U) return false;
        return true;
    }
};

struct OutgoingWindingJob
{
    static constexpr uint8_t MaxCoils = 10U;

    uint32_t jobId;
    uint32_t sessionId;
    RemoteJobType type;
    uint16_t repeatTarget;
    uint8_t coilCount;
    uint16_t turns[MaxCoils];

    OutgoingWindingJob();
    bool isValid() const;
};

enum class JobDeliveryResult : uint8_t
{
    None = 0U,
    Accepted,
    Rejected,
    TimedOut,
    Cancelled
};

struct JobDeliveryEvent
{
    static constexpr size_t MaxDetailLength = 23U;

    JobDeliveryResult result;
    uint32_t jobId;
    uint8_t sendAttempts;
    char detail[MaxDetailLength + 1U];

    JobDeliveryEvent();
};

enum class JobCancelResult : uint8_t
{
    None = 0U,
    Cancelled,
    Rejected,
    TimedOut
};

struct JobCancelEvent
{
    JobCancelResult result;
    uint32_t jobId;
    uint8_t sendAttempts;
    char detail[JobDeliveryEvent::MaxDetailLength + 1U];

    JobCancelEvent();
};

class UartEventReceiver
{
public:
    explicit UartEventReceiver(HardwareSerial& serial);

    void begin(uint32_t baud, int8_t rxPin, int8_t txPin);
    bool poll(RemoteWindingEvent& event);
    void update(uint32_t nowMs);

    bool queueJob(const OutgoingWindingJob& job);
    bool cancelPendingJob(const char* detail = "CANCELLED");
    bool takeJobDelivery(JobDeliveryEvent& event);
    bool jobPending() const;

    bool requestJobCancel(uint32_t jobId);
    bool takeJobCancel(JobCancelEvent& event);
    bool jobCancelPending() const;
    void rememberJobId(uint32_t jobId);
    void clearRecoveryJobId(uint32_t jobId)
    {
        if (jobId != 0UL && m_hasRecoveryJobId && m_recoveryJobId == jobId)
        {
            m_recoveryJobId = 0UL;
            m_hasRecoveryJobId = false;
        }
    }

    bool requestHallSettings();
    bool setHallSettings(uint16_t threshold,
                         uint16_t hysteresis,
                         uint16_t releaseDebounceMs,
                         HallSignalDirectionRemote direction);
    bool resetHallSettings();
    bool setHallTelemetryEnabled(bool enabled);
    bool armHallCalibration();
    bool abortHallCalibration();
    bool requestHallCalibration();
    bool proposeHallCalibration(uint32_t measurementId,
                                uint16_t threshold,
                                uint16_t hysteresis,
                                uint16_t releaseDebounceMs,
                                HallSignalDirectionRemote direction);
    bool hallControlPending() const;
    bool takeHallSettings(HallSettingsState& state);
    bool takeHallTelemetry(HallTelemetryState& state);
    bool takeHallCalibrationState(HallCalibrationRemoteStateSnapshot& state);
    bool takeHallCalibrationResult(HallCalibrationRemoteResult& result);
    bool takeHardwareControlReply(HardwareControlReply& reply);

    void sendAck(uint32_t runId, const char* status);
    void sendNack(uint32_t runId, const char* reason);

private:
    static constexpr size_t MaxLineLength = 192U;
    static constexpr uint32_t JobRetryIntervalMs = 2000UL;
    static constexpr uint8_t MaxJobSendAttempts = 5U;
    static constexpr uint8_t MaxCancelSendAttempts = 3U;

    bool controlLaneBusy() const;
    bool parseEventLine(char* line, RemoteWindingEvent& event) const;
    bool processJobAck(char* line);
    bool processCancelAck(char* line);
    bool processHallCalibrationRawSample(char* line);
    bool processHallCalibrationDone(char* line, uint32_t nowMs);
    bool sendPendingJob(uint32_t nowMs);
    bool writeJobFrame(const OutgoingWindingJob& job);
    bool sendPendingCancel(uint32_t nowMs);
    bool writeCancelFrame(uint32_t jobId);
    void writeRunReply(const char* category,
                       uint32_t runId,
                       const char* detail);
    void publishJobDelivery(JobDeliveryResult result,
                            uint32_t jobId,
                            uint8_t sendAttempts,
                            const char* detail);
    void publishJobCancel(JobCancelResult result,
                          uint32_t jobId,
                          uint8_t sendAttempts,
                          const char* detail);

    static bool parseDecimal32(const char* text, uint32_t& value);
    static bool parseDecimal16(const char* text, uint16_t& value);
    static bool parseHex16(const char* text, uint16_t& value);
    static bool validateAndStripJobReplyCrc(char* line);

    HardwareSerial& m_serial;
    HardwareControlClient m_hardwareControl;
    HallCalibrationRawCollector m_hallCalibrationRaw;
    bool m_hallCalibrationRawRunStarted;
    uint32_t m_hallCalibrationRawDurationMs;
    HallCalibrationRemoteResult m_compactCalibrationResult;
    bool m_hasCompactCalibrationResult;
    char m_line[MaxLineLength];
    size_t m_length;

    OutgoingWindingJob m_pendingJob;
    bool m_hasPendingJob;
    bool m_waitingJobAck;
    uint32_t m_lastJobSendMs;
    uint8_t m_jobSendAttempts;
    uint32_t m_lastQueuedJobId;
    bool m_hasLastQueuedJobId;
    uint32_t m_recoveryJobId;
    bool m_hasRecoveryJobId;
    JobDeliveryEvent m_jobDelivery;
    bool m_hasJobDelivery;

    uint32_t m_cancelJobId;
    bool m_hasPendingCancel;
    uint32_t m_lastCancelSendMs;
    uint8_t m_cancelSendAttempts;
    JobCancelEvent m_jobCancel;
    bool m_hasJobCancel;
};
}

#endif // CM_UART_EVENT_RECEIVER_H
