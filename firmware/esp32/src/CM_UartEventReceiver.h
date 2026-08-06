#ifndef CM_UART_EVENT_RECEIVER_H
#define CM_UART_EVENT_RECEIVER_H

#include <Arduino.h>

namespace CM
{
enum class RemoteEventType : uint8_t
{
    None = 0U,
    RunStarted,
    RunCompleted
};

struct RemoteWindingEvent
{
    RemoteEventType type;
    uint32_t sessionId;
    uint32_t runId;
    uint16_t completedRuns;
};

enum class RemoteJobType : uint8_t
{
    Working = 0U,
    Starting
};

struct OutgoingWindingJob
{
    static constexpr uint8_t MaxCoils = 10U;

    uint32_t jobId;
    uint32_t sessionId;
    RemoteJobType type;
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
    TimedOut
};

struct JobDeliveryEvent
{
    JobDeliveryResult result;
    uint32_t jobId;
    uint8_t sendAttempts;

    JobDeliveryEvent();
};

class UartEventReceiver
{
public:
    explicit UartEventReceiver(HardwareSerial& serial);

    void begin(uint32_t baud, int8_t rxPin, int8_t txPin);
    bool poll(RemoteWindingEvent& event);
    void update(uint32_t nowMs);

    bool queueJob(const OutgoingWindingJob& job);
    bool takeJobDelivery(JobDeliveryEvent& event);
    bool jobPending() const;

    void sendAck(uint32_t runId, const char* status);
    void sendNack(uint32_t runId, const char* reason);

private:
    static constexpr size_t MaxLineLength = 128U;
    static constexpr uint32_t JobRetryIntervalMs = 2000UL;
    static constexpr uint8_t MaxJobSendAttempts = 5U;

    bool parseEventLine(char* line, RemoteWindingEvent& event) const;
    bool processJobAck(char* line);
    bool sendPendingJob(uint32_t nowMs);
    bool writeJobFrame(const OutgoingWindingJob& job);
    void publishJobDelivery(JobDeliveryResult result,
                            uint32_t jobId,
                            uint8_t sendAttempts);

    static bool parseDecimal32(const char* text, uint32_t& value);
    static bool parseDecimal16(const char* text, uint16_t& value);
    static uint16_t crc16(const char* data, size_t length);
    static bool parseHex16(const char* text, uint16_t& value);

    HardwareSerial& m_serial;
    char m_line[MaxLineLength];
    size_t m_length;

    OutgoingWindingJob m_pendingJob;
    bool m_hasPendingJob;
    bool m_waitingJobAck;
    uint32_t m_lastJobSendMs;
    uint8_t m_jobSendAttempts;
    JobDeliveryEvent m_jobDelivery;
    bool m_hasJobDelivery;
};
}

#endif // CM_UART_EVENT_RECEIVER_H
