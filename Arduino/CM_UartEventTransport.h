#ifndef CM_UART_EVENT_TRANSPORT_H
#define CM_UART_EVENT_TRANSPORT_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "../Core/CM_WindingEvent.h"
#include "../Core/CM_WindingJob.h"
#include "CM_HardwareControlProtocol.h"
#include "CM_HallCalibrationProtocol.h"

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
    void sendJobClear();

    bool takeHardwareControlRequest(HardwareControlRequest& request);
    bool sendHardwareSettingsState(const HardwareSettings& settings,
                                   bool loadedFromEeprom);
    bool sendHardwareControlResult(HardwareControlResult result);
    bool sendHallTelemetry(const HallTelemetrySnapshot& snapshot);

    bool takeHallCalibrationCommand(HallCalibrationCommand& command);
    bool sendHallCalibrationState(HallCalibrationState state,
                                  bool baselineReady,
                                  bool motorPermit);
    bool sendHallCalibrationResult(const HallCalibrationResult& result);

    uint8_t queuedCount() const;
    bool waitingForAck() const;

private:
    static constexpr uint8_t QueueCapacity = 4U;
    // Longest supported incoming JOB/CFG frame stays below 104 bytes including
    // terminating NUL. Outgoing telemetry uses HardwareControlProtocol's own
    // bounded frame buffer.
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
    bool writeHardwareFrame(const char* frame);
    void pollReplies(uint32_t nowMs);
    void processReply(char* line, uint32_t nowMs);
    bool parseRemoteJob(char* line, WindingJob& job);
    bool parseRemoteCancel(char* line, uint32_t& jobId) const;
    void writeJobReply(bool cancelReply,
                       uint32_t jobId,
                       bool successful,
                       const char* detail);
    void removeFront();
    void publishDelivery(UartDeliveryResult result, uint32_t runId);

    static const char* eventName(WindingEventType type);
    static const char* windingTypeName(WindingType type);
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
    bool m_peerJobReplyCrcSupported;
    HardwareControlRequest m_hardwareControlRequest;
    bool m_hasHardwareControlRequest;
    HallCalibrationCommand m_hallCalibrationCommand;
    bool m_hasHallCalibrationCommand;
};
}

#endif // CM_UART_EVENT_TRANSPORT_H
