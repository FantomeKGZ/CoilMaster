#include "CM_UartEventTransport.h"

#include <stdio.h>
#include <string.h>

namespace CM
{
UartEventTransport::UartEventTransport(uint8_t rxPin,
                                       uint8_t txPin,
                                       uint32_t baudRate)
    : m_serial(rxPin, txPin),
      m_baudRate(baudRate)
{
}

void UartEventTransport::begin()
{
    m_serial.begin(m_baudRate);
}

bool UartEventTransport::send(const WindingEvent& event)
{
    if (event.type == WindingEventType::None)
    {
        return false;
    }

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

    const uint16_t crc = crc16Ccitt(
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

uint16_t UartEventTransport::crc16Ccitt(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFFU;

    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= static_cast<uint16_t>(data[index]) << 8U;

        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = static_cast<uint16_t>((crc << 1U) ^ 0x1021U);
            }
            else
            {
                crc = static_cast<uint16_t>(crc << 1U);
            }
        }
    }

    return crc;
}
}
