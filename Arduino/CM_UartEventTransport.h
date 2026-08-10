#ifndef CM_UART_EVENT_TRANSPORT_H
#define CM_UART_EVENT_TRANSPORT_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "../Core/CM_WindingEvent.h"
#include "../Core/CM_WindingJob.h"

namespace CM
{
enum class UartDeliveryResult : uint8_t
{
    None = 0U,
    Acknowledged,
    Duplicate,
    NegativeAcknowledgement
};

struct UartDeliveryEvent
{
    UartDeliveryResult result;
    uint32_t runId;

    UartDeliveryEvent()
        : result(UartDeliveryResult::None), runId(0UL)
    {
    }
};

class UartEventTransport
{
public:
    UartEventTransport(uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

    void begin();
    bool enqueue(const WindingEvent& event);
    bool enqueue(const WindingEvent& event, const WindingJob& job);
    void update(uint32_t nowMs);
    bool takeDeliveryEvent(UartDeliveryEvent& event);
    bool takeRemoteJob(WindingJob& job);
    void sendJobResult(uint32_t jobId, bool accepted, const char* reason);
    bool takeRemoteCancel(uint32_t& jobId);
    void sendJobCancelResult(uint32_t jobId, bool cancelled, const char* reason);

    uint8_t queuedCount() const;
    bool waitingForAck() const;

private:
    static constexpr uint8_t QueueCapacity = 4U;
    // Longest supported CMP1|JOB frame (10 coils, 4-digit turns, 32-bit ids)
    // stays below 104 bytes including the terminating NUL. Keeping this tight
    // returns SRAM to the Uno without reducing protocol capacity.
    static constexpr size_t MaxReplyLength = 104U;
    static constexpr uint32_t RetryIntervalMs = 1500UL;
    static constexpr uint32_t NackRetryIntervalMs = 3000UL;

    struct LocalProgramSnapshot
    {
        WindingType type;
        uint8_t coilCount;
        uint16_t targetTurns[MaxCoilsPerJob];
        bool valid;

        LocalProgramSnapshot()
            : type(WindingType::Working), coilCount(0U), targetTurns(), valid(false)
        {
        }
    };

    struct QueuedEvent
    {
        WindingEvent event;
        LocalProgramSnapshot localProgram;

        QueuedEvent() : event(), localProgram() {}
    };

    bool enqueueInternal(const WindingEvent& event,
                         const WindingJob* job);
    bool sendFront(uint32_t nowMs);
    bool writeFrame(const QueuedEvent& queued);
    bool writeStandardFrame(const WindingEvent& event);
    bool writeLocalFrame(const WindingEvent& event,
                         const LocalProgramSnapshot& program);
    void pollReplies(uint32_t nowMs);
    void processReply(char* line, uint32_t nowMs);
    bool parseRemoteJob(char* line, WindingJob& job) const;
    bool parseRemoteCancel(char* line, uint32_t& jobId) const;
    void removeFront();
    void publishDelivery(UartDeliveryResult result, uint32_t runId);

    static const char* eventName(WindingEventType type);
    static const char* windingTypeName(WindingType type);
    static uint16_t crc16Modbus(const uint8_t* data, size_t length);
    static bool parseHex16(const char* text, uint16_t& value);

    SoftwareSerial m_serial;
    uint32_t m_baudRate;
    QueuedEvent m_queue[QueueCapacity];
    uint8_t m_head;
    uint8_t m_count;
    bool m_waitingAck;
    uint32_t m_lastSendMs;
    uint32_t m_retryIntervalMs;
    char m_reply[MaxReplyLength];
    size_t m_replyLength;
    UartDeliveryEvent m_deliveryEvent;
    bool m_hasDeliveryEvent;
    WindingJob m_remoteJob;
    bool m_hasRemoteJob;
    uint32_t m_remoteCancelJobId;
    bool m_hasRemoteCancel;
};
}

#endif // CM_UART_EVENT_TRANSPORT_H
