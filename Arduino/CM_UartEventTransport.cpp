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
      m_hasDeliveryEvent(false),
      m_remoteJob(),
      m_hasRemoteJob(false),
      m_remoteCancelJobId(0UL),
      m_hasRemoteCancel(false)
{
    m_remoteJob.clear();
}

void UartEventTransport::begin()
{
    m_serial.begin(m_baudRate);
}

bool UartEventTransport::enqueue(const WindingEvent& event)
{
    return enqueueInternal(event, nullptr);
}

bool UartEventTransport::enqueue(const WindingEvent& event,
                                 const WindingJob& job)
{
    return enqueueInternal(event, &job);
}

bool UartEventTransport::enqueueInternal(const WindingEvent& event,
                                         const WindingJob* job)
{
    if (event.type == WindingEventType::None || event.sessionId == 0UL ||
        event.runId == 0UL || m_count >= QueueCapacity)
    {
        return false;
    }

    const uint8_t tail = static_cast<uint8_t>((m_head + m_count) % QueueCapacity);
    m_queue[tail] = QueuedEvent();
    m_queue[tail].event = event;
    if (job != nullptr && job->isValid() &&
        job->sessionId == event.sessionId)
    {
        m_queue[tail].job = *job;
        m_queue[tail].hasJobMetadata = true;
    }
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

bool UartEventTransport::takeRemoteJob(WindingJob& job)
{
    if (!m_hasRemoteJob)
    {
        return false;
    }

    job = m_remoteJob;
    m_remoteJob.clear();
    m_hasRemoteJob = false;
    return true;
}

void UartEventTransport::sendJobResult(uint32_t jobId,
                                       bool accepted,
                                       const char* reason)
{
    m_serial.print(F("CMP1|JOB_ACK|"));
    m_serial.print(jobId);
    m_serial.print('|');
    m_serial.print(accepted ? F("ACCEPTED") : F("REJECTED"));
    m_serial.print('|');
    m_serial.println(reason != nullptr ? reason : "NONE");
}

bool UartEventTransport::takeRemoteCancel(uint32_t& jobId)
{
    if (!m_hasRemoteCancel)
    {
        return false;
    }

    jobId = m_remoteCancelJobId;
    m_remoteCancelJobId = 0UL;
    m_hasRemoteCancel = false;
    return true;
}

void UartEventTransport::sendJobCancelResult(uint32_t jobId,
                                             bool cancelled,
                                             const char* reason)
{
    m_serial.print(F("CMP1|JOB_CANCEL_ACK|"));
    m_serial.print(jobId);
    m_serial.print('|');
    m_serial.print(cancelled ? F("CANCELLED") : F("REJECTED"));
    m_serial.print('|');
    m_serial.println(reason != nullptr ? reason : "NONE");
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

bool UartEventTransport::writeFrame(const QueuedEvent& queued)
{
    if (queued.hasJobMetadata &&
        queued.job.source == JobSource::LocalKeypad)
    {
        return writeLocalFrame(queued.event, queued.job);
    }
    return writeStandardFrame(queued.event);
}

bool UartEventTransport::writeStandardFrame(const WindingEvent& event)
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

    const uint16_t crc = crc16Modbus(
        reinterpret_cast<const uint8_t*>(payload),
        static_cast<size_t>(payloadLength));

    char frame[96];
    const int frameLength = snprintf(frame,
                                     sizeof(frame),
                                     "%s|%04X\n",
                                     payload,
                                     static_cast<unsigned int>(crc));
    if (frameLength <= 0 || static_cast<size_t>(frameLength) >= sizeof(frame))
        return false;

    return m_serial.write(reinterpret_cast<const uint8_t*>(frame),
                          static_cast<size_t>(frameLength)) ==
           static_cast<size_t>(frameLength);
}

bool UartEventTransport::writeLocalFrame(const WindingEvent& event,
                                         const WindingJob& job)
{
    if (!job.isValid() || job.source != JobSource::LocalKeypad ||
        job.sessionId != event.sessionId)
    {
        return false;
    }

    char turnsText[64];
    size_t used = 0U;
    turnsText[0] = '\0';
    for (uint8_t index = 0U; index < job.coilCount; ++index)
    {
        const int written = snprintf(turnsText + used,
                                     sizeof(turnsText) - used,
                                     index == 0U ? "%u" : ",%u",
                                     static_cast<unsigned int>(job.targetTurns[index]));
        if (written <= 0 ||
            static_cast<size_t>(written) >= sizeof(turnsText) - used)
        {
            return false;
        }
        used += static_cast<size_t>(written);
    }

    char payload[160];
    const int payloadLength = snprintf(
        payload,
        sizeof(payload),
        "CMP1|LOCAL_EVT|%s|%lu|%lu|%u|%s|%u|%s",
        eventName(event.type),
        static_cast<unsigned long>(event.sessionId),
        static_cast<unsigned long>(event.runId),
        static_cast<unsigned int>(event.completedRuns),
        windingTypeName(job.type),
        static_cast<unsigned int>(job.coilCount),
        turnsText);
    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(payload))
    {
        return false;
    }

    const uint16_t crc = crc16Modbus(
        reinterpret_cast<const uint8_t*>(payload),
        static_cast<size_t>(payloadLength));
    char frame[176];
    const int frameLength = snprintf(frame,
                                     sizeof(frame),
                                     "%s|%04X\n",
                                     payload,
                                     static_cast<unsigned int>(crc));
    if (frameLength <= 0 || static_cast<size_t>(frameLength) >= sizeof(frame))
        return false;

    return m_serial.write(reinterpret_cast<const uint8_t*>(frame),
                          static_cast<size_t>(frameLength)) ==
           static_cast<size_t>(frameLength);
}

void UartEventTransport::pollReplies(uint32_t nowMs)
{
    while (m_serial.available() > 0)
    {
        const char value = static_cast<char>(m_serial.read());
        if (value == '\r') continue;

        if (value == '\n')
        {
            m_reply[m_replyLength] = '\0';
            if (m_replyLength > 0U) processReply(m_reply, nowMs);
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
    if (strncmp(line, "CMP1|JOB_CANCEL|", 16U) == 0)
    {
        uint32_t jobId = 0UL;
        if (parseRemoteCancel(line, jobId) && !m_hasRemoteCancel)
        {
            m_remoteCancelJobId = jobId;
            m_hasRemoteCancel = true;
        }
        return;
    }

    if (strncmp(line, "CMP1|JOB|", 9U) == 0)
    {
        WindingJob parsed;
        parsed.clear();
        if (parseRemoteJob(line, parsed))
        {
            m_remoteJob = parsed;
            m_hasRemoteJob = true;
        }
        return;
    }

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
    if (runId == 0UL || runId != m_queue[m_head].event.runId)
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
        if (!accepted) return;

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

bool UartEventTransport::parseRemoteJob(char* line, WindingJob& job) const
{
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;

    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (crc16Modbus(reinterpret_cast<const uint8_t*>(line), payloadLength) !=
        receivedCrc)
    {
        return false;
    }

    *lastSeparator = '\0';
    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* jobId = strtok_r(nullptr, "|", &save);
    char* sessionId = strtok_r(nullptr, "|", &save);
    char* type = strtok_r(nullptr, "|", &save);
    char* count = strtok_r(nullptr, "|", &save);
    char* turns = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || jobId == nullptr ||
        sessionId == nullptr || type == nullptr || count == nullptr ||
        turns == nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB") != 0)
    {
        return false;
    }

    job.clear();
    job.jobId = strtoul(jobId, nullptr, 10);
    job.sessionId = strtoul(sessionId, nullptr, 10);
    job.type = strcmp(type, "STARTING") == 0 ? WindingType::Starting
                                             : WindingType::Working;
    job.source = JobSource::Esp32Web;
    job.coilCount = static_cast<uint8_t>(strtoul(count, nullptr, 10));

    if (job.jobId == 0UL || job.coilCount == 0U ||
        job.coilCount > MaxCoilsPerJob)
    {
        return false;
    }

    char* turnsSave = nullptr;
    char* token = strtok_r(turns, ",", &turnsSave);
    uint8_t index = 0U;
    while (token != nullptr && index < job.coilCount)
    {
        const unsigned long value = strtoul(token, nullptr, 10);
        if (value == 0UL || value > MaxTurnsPerCoil) return false;
        job.targetTurns[index++] = static_cast<uint16_t>(value);
        token = strtok_r(nullptr, ",", &turnsSave);
    }

    return index == job.coilCount && token == nullptr && job.isValid();
}

bool UartEventTransport::parseRemoteCancel(char* line, uint32_t& jobId) const
{
    jobId = 0UL;
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;

    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (crc16Modbus(reinterpret_cast<const uint8_t*>(line), payloadLength) !=
        receivedCrc)
    {
        return false;
    }
    *lastSeparator = '\0';

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* jobText = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || jobText == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB_CANCEL") != 0)
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = strtoul(jobText, &end, 10);
    if (end == nullptr || *end != '\0' || parsed == 0UL) return false;
    jobId = static_cast<uint32_t>(parsed);
    return true;
}

void UartEventTransport::removeFront()
{
    if (m_count == 0U) return;
    m_queue[m_head] = QueuedEvent();
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
        case WindingEventType::RunStarted: return "RUN_STARTED";
        case WindingEventType::RunCompleted: return "RUN_COMPLETED";
        case WindingEventType::None:
        default: return "NONE";
    }
}

const char* UartEventTransport::windingTypeName(WindingType type)
{
    return type == WindingType::Starting ? "STARTING" : "WORKING";
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

bool UartEventTransport::parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}
}
