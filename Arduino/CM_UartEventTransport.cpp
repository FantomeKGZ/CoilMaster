#include "CM_UartEventTransport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace CM
{
UartEventTransport::UartEventTransport(uint8_t rxPin,
                                       uint8_t txPin,
                                       uint32_t baudRate)
    : m_serial(rxPin, txPin),
      m_baudRate(baudRate),
      m_queue(),
      m_head(0U),
      m_count(0U),
      m_waitingAck(false),
      m_lastSendMs(0UL),
      m_retryIntervalMs(RetryIntervalMs),
      m_reply(),
      m_replyLength(0U),
      m_deliveryEvent(),
      m_hasDeliveryEvent(false)
{
}

void UartEventTransport::begin()
{
    m_serial.begin(m_baudRate);
}

bool UartEventTransport::enqueue(const WindingEvent& event)
{
    if (event.type == WindingEventType::None || m_count >= QueueCapacity)
    {
        return false;
    }

    const uint8_t tail = static_cast<uint8_t>((m_head + m_count) % QueueCapacity);
    m_queue[tail] = event;
    ++m_count;
    return true;
}

void UartEventTransport::update(uint32_t nowMs)
{
    pollReplies(nowMs);

    if (m_count == 0U)
    {
        m_waitingAck = false;
        return;
    }

    if (!m_waitingAck ||
        static_cast<uint32_t>(nowMs - m_lastSendMs) >= m_retryIntervalMs)
    {
        sendFront(nowMs);
    }
}

bool UartEventTransport::takeDeliveryEvent(UartDeliveryEvent& event)
{
    if (!m_hasDeliveryEvent)
    {
        return false;
    }

    event = m_deliveryEvent;
    m_deliveryEvent = UartDeliveryEvent();
    m_hasDeliveryEvent = false;
    return true;
}

uint8_t UartEventTransport::queuedCount() const
{
    return m_count;
}

bool UartEventTransport::waitingForAck() const
{
    return m_waitingAck;
}

bool UartEventTransport::sendFront(uint32_t nowMs)
{
    if (m_count == 0U || !writeFrame(m_queue[m_head]))
    {
        return false;
    }

    m_waitingAck = true;
    m_lastSendMs = nowMs;
    return true;
}

bool UartEventTransport::writeFrame(const WindingEvent& event)
{
    char payload[80];
    const int payloadLength = snprintf(
        payload,
        sizeof(payload),
        "CMP1|EVT|%s|%lu|%lu|%u",
        eventName(event.type),
        static_cast<unsigned long>(event.sessionId),
        static_cast<unsigned long>(event.runId),
        static_cast<unsigned int>(event.completedRuns));

    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(payload))
    {
        return false;
    }

    // Must match the ESP32 receiver: CRC-16/Modbus, polynomial 0xA001.
    const uint16_t crc = crc16Modbus(
        reinterpret_cast<const uint8_t*>(payload),
        static_cast<size_t>(payloadLength));

    char frame[96];
    const int frameLength = snprintf(
        frame,
        sizeof(frame),
        "%s|%04X\n",
        payload,
        static_cast<unsigned int>(crc));

    if (frameLength <= 0 ||
        static_cast<size_t>(frameLength) >= sizeof(frame))
    {
        return false;
    }

    return m_serial.write(
               reinterpret_cast<const uint8_t*>(frame),
               static_cast<size_t>(frameLength)) ==
           static_cast<size_t>(frameLength);
}

void UartEventTransport::pollReplies(uint32_t nowMs)
{
    while (m_serial.available() > 0)
    {
        const char value = static_cast<char>(m_serial.read());

        if (value == '\r')
        {
            continue;
        }

        if (value == '\n')
        {
            m_reply[m_replyLength] = '\0';
            if (m_replyLength > 0U)
            {
                processReply(m_reply, nowMs);
            }
            m_replyLength = 0U;
            continue;
        }

        if (m_replyLength + 1U >= MaxReplyLength)
        {
            m_replyLength = 0U;
            continue;
        }

        m_reply[m_replyLength++] = value;
    }
}

void UartEventTransport::processReply(char* line, uint32_t nowMs)
{
    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* runText = strtok_r(nullptr, "|", &save);
    char* status = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr ||
        runText == nullptr || status == nullptr ||
        strcmp(version, "CMP1") != 0 || m_count == 0U)
    {
        return;
    }

    const uint32_t runId = strtoul(runText, nullptr, 10);
    if (runId == 0UL || runId != m_queue[m_head].runId)
    {
        return;
    }

    if (strcmp(category, "ACK") == 0)
    {
        const bool duplicate = strcmp(status, "DUPLICATE") == 0;
        const bool accepted = duplicate ||
                              strcmp(status, "RECORDED") == 0 ||
                              strcmp(status, "SAVED") == 0 ||
                              strcmp(status, "RECEIVED") == 0;
        if (!accepted)
        {
            return;
        }

        publishDelivery(duplicate ? UartDeliveryResult::Duplicate
                                  : UartDeliveryResult::Acknowledged,
                        runId);
        removeFront();
        m_waitingAck = false;
        m_retryIntervalMs = RetryIntervalMs;
        m_lastSendMs = nowMs;
        return;
    }

    if (strcmp(category, "NACK") == 0)
    {
        publishDelivery(UartDeliveryResult::NegativeAcknowledgement, runId);
        m_waitingAck = true;
        m_retryIntervalMs = NackRetryIntervalMs;
        m_lastSendMs = nowMs;
    }
}

void UartEventTransport::removeFront()
{
    if (m_count == 0U)
    {
        return;
    }

    m_head = static_cast<uint8_t>((m_head + 1U) % QueueCapacity);
    --m_count;
}

void UartEventTransport::publishDelivery(UartDeliveryResult result,
                                         uint32_t runId)
{
    m_deliveryEvent.result = result;
    m_deliveryEvent.runId = runId;
    m_hasDeliveryEvent = true;
}

const char* UartEventTransport::eventName(WindingEventType type)
{
    switch (type)
    {
        case WindingEventType::RunStarted:
            return "RUN_STARTED";

        case WindingEventType::RunCompleted:
            return "RUN_COMPLETED";

        case WindingEventType::None:
        default:
            return "NONE";
    }
}

uint16_t UartEventTransport::crc16Modbus(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFFU;

    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 1U) != 0U
                      ? static_cast<uint16_t>((crc >> 1U) ^ 0xA001U)
                      : static_cast<uint16_t>(crc >> 1U);
        }
    }

    return crc;
}
}
