#include "CM_UartEventReceiver.h"

#include <stdlib.h>
#include <string.h>

namespace CM
{
UartEventReceiver::UartEventReceiver(HardwareSerial& serial)
    : m_serial(serial), m_line(), m_length(0U)
{
}

void UartEventReceiver::begin(uint32_t baud, int8_t rxPin, int8_t txPin)
{
    m_serial.begin(baud, SERIAL_8N1, rxPin, txPin);
}

bool UartEventReceiver::poll(RemoteWindingEvent& event)
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
            m_line[m_length] = '\0';
            const bool valid = m_length > 0U && parseLine(m_line, event);
            m_length = 0U;
            return valid;
        }

        if (m_length + 1U >= MaxLineLength)
        {
            m_length = 0U;
            sendNack(0UL, "LINE_TOO_LONG");
            continue;
        }

        m_line[m_length++] = value;
    }

    return false;
}

void UartEventReceiver::sendAck(const RemoteWindingEvent& event)
{
    m_serial.print(F("CMP1|ACK|"));
    m_serial.print(event.runId);
    m_serial.print('|');
    m_serial.println(event.type == RemoteEventType::RunCompleted
                         ? F("SAVED")
                         : F("RECEIVED"));
}

void UartEventReceiver::sendNack(uint32_t runId, const char* reason)
{
    m_serial.print(F("CMP1|NACK|"));
    m_serial.print(runId);
    m_serial.print('|');
    m_serial.println(reason != nullptr ? reason : "ERROR");
}

bool UartEventReceiver::parseLine(char* line, RemoteWindingEvent& event) const
{
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr)
    {
        return false;
    }

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc))
    {
        return false;
    }

    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (crc16(line, payloadLength) != receivedCrc)
    {
        return false;
    }

    *lastSeparator = '\0';

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* type = strtok_r(nullptr, "|", &save);
    char* session = strtok_r(nullptr, "|", &save);
    char* run = strtok_r(nullptr, "|", &save);
    char* completed = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || type == nullptr ||
        session == nullptr || run == nullptr || completed == nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "EVT") != 0)
    {
        return false;
    }

    if (strcmp(type, "RUN_STARTED") == 0)
    {
        event.type = RemoteEventType::RunStarted;
    }
    else if (strcmp(type, "RUN_COMPLETED") == 0)
    {
        event.type = RemoteEventType::RunCompleted;
    }
    else
    {
        return false;
    }

    event.sessionId = strtoul(session, nullptr, 10);
    event.runId = strtoul(run, nullptr, 10);
    event.completedRuns = static_cast<uint16_t>(strtoul(completed, nullptr, 10));
    return event.sessionId > 0UL && event.runId > 0UL;
}

uint16_t UartEventReceiver::crc16(const char* data, size_t length)
{
    uint16_t crc = 0xFFFFU;

    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= static_cast<uint8_t>(data[index]);
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 1U) != 0U
                      ? static_cast<uint16_t>((crc >> 1U) ^ 0xA001U)
                      : static_cast<uint16_t>(crc >> 1U);
        }
    }

    return crc;
}

bool UartEventReceiver::parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U)
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL)
    {
        return false;
    }

    value = static_cast<uint16_t>(parsed);
    return true;
}
}
