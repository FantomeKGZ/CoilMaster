#include "CM_UartEventReceiver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace CM
{
namespace
{
bool isValidJobAckDetail(const char* detail)
{
    if (detail == nullptr) return true;

    const size_t length = strlen(detail);
    if (length == 0U || length > JobDeliveryEvent::MaxDetailLength)
        return false;

    for (size_t index = 0U; index < length; ++index)
    {
        const char value = detail[index];
        const bool upper = value >= 'A' && value <= 'Z';
        const bool digit = value >= '0' && value <= '9';
        if (!upper && !digit && value != '_' && value != '-')
            return false;
    }
    return true;
}
}

OutgoingWindingJob::OutgoingWindingJob()
    : jobId(0UL), sessionId(0UL), type(RemoteJobType::Working),
      coilCount(0U), turns()
{
}

bool OutgoingWindingJob::isValid() const
{
    if (jobId == 0UL || sessionId == 0UL ||
        coilCount == 0U || coilCount > MaxCoils)
        return false;

    for (uint8_t index = 0U; index < coilCount; ++index)
        if (turns[index] == 0U || turns[index] > 9999U) return false;
    return true;
}

JobDeliveryEvent::JobDeliveryEvent()
    : result(JobDeliveryResult::None), jobId(0UL), sendAttempts(0U), detail()
{
    detail[0] = '\0';
}

UartEventReceiver::UartEventReceiver(HardwareSerial& serial)
    : m_serial(serial), m_line(), m_length(0U), m_pendingJob(),
      m_hasPendingJob(false), m_waitingJobAck(false),
      m_lastJobSendMs(0UL), m_jobSendAttempts(0U),
      m_jobDelivery(), m_hasJobDelivery(false)
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
        if (value == '\r') continue;
        if (value == '\n')
        {
            m_line[m_length] = '\0';
            bool eventReady = false;
            if (m_length > 0U)
            {
                if (strncmp(m_line, "CMP1|JOB_ACK|", 13U) == 0)
                    processJobAck(m_line);
                else
                    eventReady = parseEventLine(m_line, event);
            }
            m_length = 0U;
            if (eventReady) return true;
            continue;
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

void UartEventReceiver::update(uint32_t nowMs)
{
    if (!m_hasPendingJob) return;
    if (!m_waitingJobAck)
    {
        sendPendingJob(nowMs);
        return;
    }
    if (static_cast<uint32_t>(nowMs - m_lastJobSendMs) < JobRetryIntervalMs)
        return;
    if (m_jobSendAttempts >= MaxJobSendAttempts)
    {
        publishJobDelivery(JobDeliveryResult::TimedOut,
                           m_pendingJob.jobId,
                           m_jobSendAttempts,
                           "NO_ACK");
        m_hasPendingJob = false;
        m_waitingJobAck = false;
        m_jobSendAttempts = 0U;
        return;
    }
    sendPendingJob(nowMs);
}

bool UartEventReceiver::queueJob(const OutgoingWindingJob& job)
{
    if (m_hasPendingJob || m_hasJobDelivery || !job.isValid()) return false;
    m_pendingJob = job;
    m_hasPendingJob = true;
    m_waitingJobAck = false;
    m_jobSendAttempts = 0U;
    return true;
}

bool UartEventReceiver::takeJobDelivery(JobDeliveryEvent& event)
{
    if (!m_hasJobDelivery) return false;
    event = m_jobDelivery;
    m_jobDelivery = JobDeliveryEvent();
    m_hasJobDelivery = false;
    return true;
}

bool UartEventReceiver::jobPending() const { return m_hasPendingJob; }

void UartEventReceiver::sendAck(uint32_t runId, const char* status)
{
    m_serial.print(F("CMP1|ACK|"));
    m_serial.print(runId);
    m_serial.print('|');
    m_serial.println(status != nullptr ? status : "RECEIVED");
}

void UartEventReceiver::sendNack(uint32_t runId, const char* reason)
{
    m_serial.print(F("CMP1|NACK|"));
    m_serial.print(runId);
    m_serial.print('|');
    m_serial.println(reason != nullptr ? reason : "ERROR");
}

bool UartEventReceiver::parseEventLine(char* line,
                                       RemoteWindingEvent& event) const
{
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;
    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;
    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (crc16(line, payloadLength) != receivedCrc) return false;
    *lastSeparator = '\0';

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* type = strtok_r(nullptr, "|", &save);
    char* session = strtok_r(nullptr, "|", &save);
    char* run = strtok_r(nullptr, "|", &save);
    char* completed = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || type == nullptr ||
        session == nullptr || run == nullptr || completed == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "EVT") != 0)
        return false;

    RemoteEventType parsedType = RemoteEventType::None;
    if (strcmp(type, "RUN_STARTED") == 0)
        parsedType = RemoteEventType::RunStarted;
    else if (strcmp(type, "RUN_COMPLETED") == 0)
        parsedType = RemoteEventType::RunCompleted;
    else
        return false;

    uint32_t parsedSession = 0UL;
    uint32_t parsedRun = 0UL;
    uint16_t parsedCompleted = 0U;
    if (!parseDecimal32(session, parsedSession) || parsedSession == 0UL ||
        !parseDecimal32(run, parsedRun) || parsedRun == 0UL ||
        !parseDecimal16(completed, parsedCompleted))
        return false;
    if ((parsedType == RemoteEventType::RunStarted && parsedCompleted != 0U) ||
        (parsedType == RemoteEventType::RunCompleted && parsedCompleted == 0U))
        return false;

    event.type = parsedType;
    event.sessionId = parsedSession;
    event.runId = parsedRun;
    event.completedRuns = parsedCompleted;
    return true;
}

bool UartEventReceiver::processJobAck(char* line)
{
    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* jobText = strtok_r(nullptr, "|", &save);
    char* status = strtok_r(nullptr, "|", &save);
    char* detail = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || jobText == nullptr ||
        status == nullptr || extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB_ACK") != 0 || !m_hasPendingJob ||
        !isValidJobAckDetail(detail))
        return false;

    uint32_t jobId = 0UL;
    if (!parseDecimal32(jobText, jobId) || jobId != m_pendingJob.jobId)
        return false;

    JobDeliveryResult result = JobDeliveryResult::None;
    if (strcmp(status, "ACCEPTED") == 0)
        result = JobDeliveryResult::Accepted;
    else if (strcmp(status, "REJECTED") == 0)
    {
        if (detail == nullptr) return false;
        result = JobDeliveryResult::Rejected;
    }
    else
        return false;

    publishJobDelivery(result,
                       jobId,
                       m_jobSendAttempts,
                       detail != nullptr ? detail :
                       (result == JobDeliveryResult::Accepted ? "ACCEPTED" : "REJECTED"));
    m_hasPendingJob = false;
    m_waitingJobAck = false;
    m_jobSendAttempts = 0U;
    return true;
}

bool UartEventReceiver::sendPendingJob(uint32_t nowMs)
{
    if (!m_hasPendingJob || m_jobSendAttempts >= MaxJobSendAttempts ||
        !writeJobFrame(m_pendingJob))
        return false;
    ++m_jobSendAttempts;
    m_waitingJobAck = true;
    m_lastJobSendMs = nowMs;
    return true;
}

bool UartEventReceiver::writeJobFrame(const OutgoingWindingJob& job)
{
    char turnsText[64];
    size_t used = 0U;
    turnsText[0] = '\0';
    for (uint8_t index = 0U; index < job.coilCount; ++index)
    {
        const int written = snprintf(turnsText + used, sizeof(turnsText) - used,
                                     index == 0U ? "%u" : ",%u",
                                     static_cast<unsigned int>(job.turns[index]));
        if (written <= 0 || static_cast<size_t>(written) >= sizeof(turnsText) - used)
            return false;
        used += static_cast<size_t>(written);
    }

    char payload[112];
    const int payloadLength = snprintf(
        payload, sizeof(payload), "CMP1|JOB|%lu|%lu|%s|%u|%s",
        static_cast<unsigned long>(job.jobId),
        static_cast<unsigned long>(job.sessionId),
        job.type == RemoteJobType::Starting ? "STARTING" : "WORKING",
        static_cast<unsigned int>(job.coilCount), turnsText);
    if (payloadLength <= 0 || static_cast<size_t>(payloadLength) >= sizeof(payload))
        return false;

    const uint16_t crc = crc16(payload, static_cast<size_t>(payloadLength));
    m_serial.print(payload);
    m_serial.print('|');
    if (crc < 0x1000U) m_serial.print('0');
    if (crc < 0x0100U) m_serial.print('0');
    if (crc < 0x0010U) m_serial.print('0');
    m_serial.println(crc, HEX);
    return true;
}

void UartEventReceiver::publishJobDelivery(JobDeliveryResult result,
                                           uint32_t jobId,
                                           uint8_t sendAttempts,
                                           const char* detail)
{
    m_jobDelivery.result = result;
    m_jobDelivery.jobId = jobId;
    m_jobDelivery.sendAttempts = sendAttempts;
    const char* source = detail != nullptr ? detail : "";
    strncpy(m_jobDelivery.detail, source, JobDeliveryEvent::MaxDetailLength);
    m_jobDelivery.detail[JobDeliveryEvent::MaxDetailLength] = '\0';
    m_hasJobDelivery = true;
}

uint16_t UartEventReceiver::crc16(const char* data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= static_cast<uint8_t>(data[index]);
        for (uint8_t bit = 0U; bit < 8U; ++bit)
            crc = (crc & 1U) != 0U
                      ? static_cast<uint16_t>((crc >> 1U) ^ 0xA001U)
                      : static_cast<uint16_t>(crc >> 1U);
    }
    return crc;
}

bool UartEventReceiver::parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool UartEventReceiver::parseDecimal32(const char* text, uint32_t& value)
{
    value = 0UL;
    if (text == nullptr || *text == '\0') return false;
    uint64_t parsed = 0ULL;
    for (const char* cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor < '0' || *cursor > '9') return false;
        parsed = parsed * 10ULL + static_cast<uint8_t>(*cursor - '0');
        if (parsed > 0xFFFFFFFFULL) return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool UartEventReceiver::parseDecimal16(const char* text, uint16_t& value)
{
    uint32_t parsed = 0UL;
    if (!parseDecimal32(text, parsed) || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}
}
