#ifndef CM_UART_EVENT_TRANSPORT_H
#define CM_UART_EVENT_TRANSPORT_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "../Core/CM_WindingEvent.h"

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
    void update(uint32_t nowMs);
    bool takeDeliveryEvent(UartDeliveryEvent& event);

    uint8_t queuedCount() const;
    bool waitingForAck() const;

private:
    static constexpr uint8_t QueueCapacity = 4U;
    static constexpr size_t MaxReplyLength = 64U;
    static constexpr uint32_t RetryIntervalMs = 1500UL;
    static constexpr uint32_t NackRetryIntervalMs = 3000UL;

    bool sendFront(uint32_t nowMs);
    bool writeFrame(const WindingEvent& event);
    void pollReplies(uint32_t nowMs);
    void processReply(char* line, uint32_t nowMs);
    void removeFront();
    void publishDelivery(UartDeliveryResult result, uint32_t runId);

    static const char* eventName(WindingEventType type);
    static uint16_t crc16Modbus(const uint8_t* data, size_t length);

    SoftwareSerial m_serial;
    uint32_t m_baudRate;
    WindingEvent m_queue[QueueCapacity];
    uint8_t m_head;
    uint8_t m_count;
    bool m_waitingAck;
    uint32_t m_lastSendMs;
    uint32_t m_retryIntervalMs;
    char m_reply[MaxReplyLength];
    size_t m_replyLength;
    UartDeliveryEvent m_deliveryEvent;
    bool m_hasDeliveryEvent;
};
}

#endif // CM_UART_EVENT_TRANSPORT_H
