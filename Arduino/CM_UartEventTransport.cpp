#include "CM_UartEventTransport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{
namespace
{
bool parseCanonicalUnsigned(const char* text,
                            uint32_t maximum,
                            uint32_t& value)
{
    value = 0UL;
    if (text == nullptr || *text == '\0') return false;
    if (text[0] == '0' && text[1] != '\0') return false;

    const uint32_t quotient = maximum / 10UL;
    const uint8_t remainder = static_cast<uint8_t>(maximum % 10UL);
    for (const char* cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor < '0' || *cursor > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(*cursor - '0');
        if (value > quotient || (value == quotient && digit > remainder))
            return false;
        value = value * 10UL + digit;
    }
    return true;
}
}

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
      m_hasRemoteCancel(false),
      m_peerJobReplyCrcSupported(false),
      m_hardwareControlRequest(),
      m_hasHardwareControlRequest(false),
      m_hallCalibrationCommand(HallCalibrationCommand::None),
      m_hasHallCalibrationCommand(false)
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
        job->source == JobSource::LocalKeypad &&
        job->sessionId == event.sessionId)
    {
        LocalProgramSnapshot& snapshot = m_queue[tail].localProgram;
        snapshot.type = job->type;
        snapshot.coilCount = job->coilCount;
        for (uint8_t index = 0U; index < job->coilCount; ++index)
            snapshot.targetTurns[index] = job->targetTurns[index];
        snapshot.valid = true;
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
    if (!m_hasDeliveryEvent) return false;

    event = m_deliveryEvent;
    m_deliveryEvent = UartDeliveryEvent();
    m_hasDeliveryEvent = false;
    return true;
}

bool UartEventTransport::takeRemoteJob(WindingJob& job)
{
    if (!m_hasRemoteJob) return false;

    job = m_remoteJob;
    m_remoteJob.clear();
    m_hasRemoteJob = false;
    return true;
}

void UartEventTransport::sendJobResult(uint32_t jobId,
                                       bool accepted,
                                       const char* reason)
{
    writeJobReply(false, jobId, accepted, reason);
}

bool UartEventTransport::takeRemoteCancel(uint32_t& jobId)
{
    if (!m_hasRemoteCancel) return false;

    jobId = m_remoteCancelJobId;
    m_remoteCancelJobId = 0UL;
    m_hasRemoteCancel = false;
    return true;
}

void UartEventTransport::sendJobCancelResult(uint32_t jobId,
                                             bool cancelled,
                                             const char* reason)
{
    writeJobReply(true, jobId, cancelled, reason);
}

void UartEventTransport::sendJobClear()
{
    char frame[64];
    const int payloadLength = snprintf_P(
        frame, sizeof(frame),
        PSTR("CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C"));
    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(frame)) return;

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(frame),
        static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf_P(
        frame + payloadLength,
        sizeof(frame) - static_cast<size_t>(payloadLength),
        PSTR("|%04X\n"), static_cast<unsigned int>(crc));
    if (suffixLength <= 0 ||
        static_cast<size_t>(suffixLength) >=
            sizeof(frame) - static_cast<size_t>(payloadLength)) return;

    const size_t frameLength = static_cast<size_t>(payloadLength + suffixLength);
    m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength);
}

bool UartEventTransport::takeHardwareControlRequest(
    HardwareControlRequest& request)
{
    if (!m_hasHardwareControlRequest) return false;

    request = m_hardwareControlRequest;
    m_hardwareControlRequest = HardwareControlRequest();
    m_hasHardwareControlRequest = false;
    return true;
}

bool UartEventTransport::sendHardwareSettingsState(
    const HardwareSettings& settings,
    bool loadedFromEeprom)
{
    char frame[HardwareControlProtocol::MaxFrameLength];
    if (!HardwareControlProtocol::formatSettingsState(
            settings, loadedFromEeprom, frame, sizeof(frame)))
    {
        return false;
    }
    return writeHardwareFrame(frame);
}

bool UartEventTransport::sendHardwareControlResult(
    HardwareControlResult result)
{
    char frame[HardwareControlProtocol::MaxFrameLength];
    if (!HardwareControlProtocol::formatSettingsResult(
            result, frame, sizeof(frame)))
    {
        return false;
    }
    return writeHardwareFrame(frame);
}

bool UartEventTransport::sendHallTelemetry(
    const HallTelemetrySnapshot& snapshot)
{
    char frame[HardwareControlProtocol::MaxFrameLength];
    if (!HardwareControlProtocol::formatHallTelemetry(
            snapshot, frame, sizeof(frame)))
    {
        return false;
    }
    return writeHardwareFrame(frame);
}

bool UartEventTransport::takeHallCalibrationCommand(
    HallCalibrationCommand& command)
{
    if (!m_hasHallCalibrationCommand) return false;
    command = m_hallCalibrationCommand;
    m_hallCalibrationCommand = HallCalibrationCommand::None;
    m_hasHallCalibrationCommand = false;
    return true;
}

bool UartEventTransport::sendHallCalibrationState(
    HallCalibrationState state, bool baselineReady, bool motorPermit)
{
    char frame[HallCalibrationProtocol::MaxFrameLength];
    if (!HallCalibrationProtocol::formatState(
            state, baselineReady, motorPermit, frame, sizeof(frame)))
    {
        return false;
    }
    return writeHardwareFrame(frame);
}

bool UartEventTransport::sendHallCalibrationResult(
    const HallCalibrationResult& result)
{
    char frame[HallCalibrationProtocol::MaxFrameLength];
    if (!HallCalibrationProtocol::formatResult(result, frame, sizeof(frame)))
    {
        return false;
    }
    return writeHardwareFrame(frame);
}

void UartEventTransport::writeJobReply(bool cancelReply,
                                       uint32_t jobId,
                                       bool successful,
                                       const char* detail)
{
    if (!m_peerJobReplyCrcSupported)
    {
        m_serial.print(cancelReply ? F("CMP1|JOB_CANCEL_ACK|")
                                   : F("CMP1|JOB_ACK|"));
        m_serial.print(jobId);
        m_serial.print('|');
        if (successful)
            m_serial.print(cancelReply ? F("CANCELLED") : F("ACCEPTED"));
        else
            m_serial.print(F("REJECTED"));
        m_serial.print('|');
        if (detail != nullptr)
            m_serial.println(detail);
        else
            m_serial.println(F("NONE"));
        return;
    }

    char frame[88];
    PGM_P format;
    if (cancelReply)
        format = successful
                     ? (detail != nullptr
                            ? PSTR("CMP1|JOB_CANCEL_ACK|%lu|CANCELLED|%s|C")
                            : PSTR("CMP1|JOB_CANCEL_ACK|%lu|CANCELLED|NONE|C"))
                     : (detail != nullptr
                            ? PSTR("CMP1|JOB_CANCEL_ACK|%lu|REJECTED|%s|C")
                            : PSTR("CMP1|JOB_CANCEL_ACK|%lu|REJECTED|NONE|C"));
    else
        format = successful
                     ? (detail != nullptr
                            ? PSTR("CMP1|JOB_ACK|%lu|ACCEPTED|%s|C")
                            : PSTR("CMP1|JOB_ACK|%lu|ACCEPTED|NONE|C"))
                     : (detail != nullptr
                            ? PSTR("CMP1|JOB_ACK|%lu|REJECTED|%s|C")
                            : PSTR("CMP1|JOB_ACK|%lu|REJECTED|NONE|C"));

    const int payloadLength = detail != nullptr
                                  ? snprintf_P(
                                        frame, sizeof(frame), format,
                                        static_cast<unsigned long>(jobId), detail)
                                  : snprintf_P(
                                        frame, sizeof(frame), format,
                                        static_cast<unsigned long>(jobId));
    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(frame)) return;

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(frame),
        static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf_P(
        frame + payloadLength,
        sizeof(frame) - static_cast<size_t>(payloadLength),
        PSTR("|%04X\n"), static_cast<unsigned int>(crc));
    if (suffixLength <= 0 ||
        static_cast<size_t>(suffixLength) >=
            sizeof(frame) - static_cast<size_t>(payloadLength)) return;

    const size_t frameLength = static_cast<size_t>(payloadLength + suffixLength);
    m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength);
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
    if (m_count == 0U || !writeFrame(m_queue[m_head])) return false;

    m_waitingAck = true;
    m_lastSendMs = nowMs;
    return true;
}

bool UartEventTransport::writeFrame(const QueuedEvent& queued)
{
    if (queued.localProgram.valid)
        return writeLocalFrame(queued.event, queued.localProgram);
    return writeStandardFrame(queued.event);
}

bool UartEventTransport::writeStandardFrame(const WindingEvent& event)
{
    // Build payload and CRC in one buffer to keep Uno stack usage small.
    char frame[96];
    const int payloadLength = snprintf(
        frame,
        sizeof(frame),
        "CMP1|EVT|%s|%lu|%lu|%u",
        eventName(event.type),
        static_cast<unsigned long>(event.sessionId),
        static_cast<unsigned long>(event.runId),
        static_cast<unsigned int>(event.completedRuns));
    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(frame))
    {
        return false;
    }

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(frame),
        static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf(
        frame + payloadLength,
        sizeof(frame) - static_cast<size_t>(payloadLength),
        "|%04X\n",
        static_cast<unsigned int>(crc));
    if (suffixLength <= 0 ||
        static_cast<size_t>(payloadLength + suffixLength) >= sizeof(frame))
    {
        return false;
    }

    const size_t frameLength = static_cast<size_t>(payloadLength + suffixLength);
    return m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength) ==
           frameLength;
}

bool UartEventTransport::writeLocalFrame(
    const WindingEvent& event,
    const LocalProgramSnapshot& program)
{
    if (!program.valid || program.coilCount == 0U ||
        program.coilCount > MaxCoilsPerJob)
    {
        return false;
    }

    // One buffer is enough for the largest CMP1 LOCAL_EVT frame. Avoid the old
    // turnsText + payload + frame triple allocation, which consumed ~400 bytes
    // of Uno stack during a send.
    char frame[176];
    int used = snprintf(
        frame,
        sizeof(frame),
        "CMP1|LOCAL_EVT|%s|%lu|%lu|%u|%s|%u|",
        eventName(event.type),
        static_cast<unsigned long>(event.sessionId),
        static_cast<unsigned long>(event.runId),
        static_cast<unsigned int>(event.completedRuns),
        windingTypeName(program.type),
        static_cast<unsigned int>(program.coilCount));
    if (used <= 0 || static_cast<size_t>(used) >= sizeof(frame)) return false;

    for (uint8_t index = 0U; index < program.coilCount; ++index)
    {
        const uint16_t turns = program.targetTurns[index];
        if (turns == 0U || turns > MaxTurnsPerCoil) return false;

        const int written = snprintf(
            frame + used,
            sizeof(frame) - static_cast<size_t>(used),
            index == 0U ? "%u" : ",%u",
            static_cast<unsigned int>(turns));
        if (written <= 0 ||
            static_cast<size_t>(written) >=
                sizeof(frame) - static_cast<size_t>(used))
        {
            return false;
        }
        used += written;
    }

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(frame),
        static_cast<size_t>(used));
    const int suffixLength = snprintf(
        frame + used,
        sizeof(frame) - static_cast<size_t>(used),
        "|%04X\n",
        static_cast<unsigned int>(crc));
    if (suffixLength <= 0 ||
        static_cast<size_t>(suffixLength) >=
            sizeof(frame) - static_cast<size_t>(used))
    {
        return false;
    }

    const size_t frameLength = static_cast<size_t>(used + suffixLength);
    return m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength) ==
           frameLength;
}

bool UartEventTransport::writeHardwareFrame(const char* frame)
{
    if (frame == nullptr || *frame == '\0') return false;
    const size_t frameLength = strlen(frame);
    if (frameLength == 0U || frameLength >= HardwareControlProtocol::MaxFrameLength)
        return false;
    return m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength) ==
           frameLength;
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
    if (strncmp(line, "CMP1|CAL|", 9U) == 0)
    {
        HallCalibrationCommand parsed = HallCalibrationCommand::None;
        if (HallCalibrationProtocol::parseRequest(line, parsed) &&
            !m_hasHallCalibrationCommand)
        {
            m_hallCalibrationCommand = parsed;
            m_hasHallCalibrationCommand = true;
        }
        return;
    }

    const bool hardwareControlFrame =
        strncmp(line, "CMP1|CFG_GET|", 13U) == 0 ||
        strncmp(line, "CMP1|CFG_SET|", 13U) == 0 ||
        strncmp(line, "CMP1|CFG_RESET|", 15U) == 0 ||
        strncmp(line, "CMP1|HALL_TELEM|", 16U) == 0;
    if (hardwareControlFrame)
    {
        HardwareControlRequest parsed;
        if (HardwareControlProtocol::parseRequest(line, parsed) &&
            !m_hasHardwareControlRequest)
        {
            m_hardwareControlRequest = parsed;
            m_hasHardwareControlRequest = true;
        }
        return;
    }

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

    uint8_t separatorCount = 0U;
    for (const char* value = line; *value != '\0'; ++value)
        if (*value == '|') ++separatorCount;

    // Current ACK/NACK has a fourth separator followed by CRC. Keep accepting
    // the three-separator legacy reply during staged ESP32/Arduino upgrades.
    if (separatorCount == 4U)
    {
        char* lastSeparator = strrchr(line, '|');
        uint16_t receivedCrc = 0U;
        if (lastSeparator == nullptr ||
            !parseHex16(lastSeparator + 1, receivedCrc)) return;
        const size_t payloadLength =
            static_cast<size_t>(lastSeparator - line);
        if (Cmp1Crc::calculate(
                reinterpret_cast<const uint8_t*>(line), payloadLength) !=
            receivedCrc) return;
        *lastSeparator = '\0';
    }
    else if (separatorCount != 3U) return;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* runText = strtok_r(nullptr, "|", &save);
    char* status = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr ||
        runText == nullptr || status == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || m_count == 0U)
    {
        return;
    }

    const uint32_t runId = strtoul(runText, nullptr, 10);
    if (runId == 0UL || runId != m_queue[m_head].event.runId) return;

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

bool UartEventTransport::parseRemoteJob(char* line, WindingJob& job)
{
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;

    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(reinterpret_cast<const uint8_t*>(line), payloadLength) !=
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
    char* repeatOrCapability = strtok_r(nullptr, "|", &save);
    char* capability = repeatOrCapability;
    char* extra = nullptr;

    job.clear();
    if (repeatOrCapability != nullptr && repeatOrCapability[0] == 'R')
    {
        uint32_t parsedRepeat = 0UL;
        if (!parseCanonicalUnsigned(repeatOrCapability + 1,
                                    0xFFFFUL,
                                    parsedRepeat) ||
            parsedRepeat == 0UL)
        {
            return false;
        }
        job.repeatTarget = static_cast<uint16_t>(parsedRepeat);
        capability = strtok_r(nullptr, "|", &save);
    }
    extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || jobId == nullptr ||
        sessionId == nullptr || type == nullptr || count == nullptr ||
        turns == nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB") != 0 || extra != nullptr ||
        (capability != nullptr && strcmp(capability, "C") != 0))
    {
        return false;
    }

    uint32_t parsedJobId = 0UL;
    uint32_t parsedSessionId = 0UL;
    uint32_t parsedCoilCount = 0UL;
    if (!parseCanonicalUnsigned(jobId, 0xFFFFFFFFUL, parsedJobId) ||
        !parseCanonicalUnsigned(sessionId, 0xFFFFFFFFUL, parsedSessionId) ||
        !parseCanonicalUnsigned(count, MaxCoilsPerJob, parsedCoilCount) ||
        parsedJobId == 0UL || parsedSessionId == 0UL ||
        parsedCoilCount == 0UL)
    {
        return false;
    }

    if (strcmp(type, "STARTING") == 0)
        job.type = WindingType::Starting;
    else if (strcmp(type, "WORKING") == 0)
        job.type = WindingType::Working;
    else
        return false;

    job.jobId = parsedJobId;
    job.sessionId = parsedSessionId;
    job.source = JobSource::Esp32Web;
    job.coilCount = static_cast<uint8_t>(parsedCoilCount);

    char* turnsSave = nullptr;
    char* token = strtok_r(turns, ",", &turnsSave);
    uint8_t index = 0U;
    while (token != nullptr && index < job.coilCount)
    {
        uint32_t value = 0UL;
        if (!parseCanonicalUnsigned(token, MaxTurnsPerCoil, value) ||
            value == 0UL)
        {
            return false;
        }
        job.targetTurns[index++] = static_cast<uint16_t>(value);
        token = strtok_r(nullptr, ",", &turnsSave);
    }

    const bool valid = index == job.coilCount && token == nullptr && job.isValid();
    if (valid) m_peerJobReplyCrcSupported = capability != nullptr;
    return valid;
}

bool UartEventTransport::parseRemoteCancel(char* line, uint32_t& jobId) const
{
    jobId = 0UL;
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;

    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(reinterpret_cast<const uint8_t*>(line), payloadLength) !=
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
