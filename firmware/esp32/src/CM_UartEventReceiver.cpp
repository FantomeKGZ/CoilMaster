#include "CM_UartEventReceiver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../Shared/CMP1Text/CM_Cmp1Crc.h"

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
        if (!upper && !digit && value != '_' && value != '-') return false;
    }
    return true;
}

bool isValidRunReplyDetail(const char* detail)
{
    if (detail == nullptr) return false;
    const size_t length = strlen(detail);
    if (length == 0U || length > 32U) return false;
    for (size_t index = 0U; index < length; ++index)
    {
        const char value = detail[index];
        const bool upper = value >= 'A' && value <= 'Z';
        const bool digit = value >= '0' && value <= '9';
        if (!upper && !digit && value != '_' && value != '-') return false;
    }
    return true;
}

bool parseEventType(const char* text, RemoteEventType& type)
{
    type = RemoteEventType::None;
    if (text == nullptr) return false;
    if (strcmp(text, "RUN_STARTED") == 0)
        type = RemoteEventType::RunStarted;
    else if (strcmp(text, "RUN_COMPLETED") == 0)
        type = RemoteEventType::RunCompleted;
    else
        return false;
    return true;
}
}

OutgoingWindingJob::OutgoingWindingJob()
    : jobId(0UL), sessionId(0UL), type(RemoteJobType::Working),
      repeatTarget(1U), coilCount(0U), turns()
{
}

bool OutgoingWindingJob::isValid() const
{
    if (jobId == 0UL || sessionId == 0UL || repeatTarget == 0U ||
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

JobCancelEvent::JobCancelEvent()
    : result(JobCancelResult::None), jobId(0UL), sendAttempts(0U), detail()
{
    detail[0] = '\0';
}

UartEventReceiver::UartEventReceiver(HardwareSerial& serial)
    : m_serial(serial), m_hardwareControl(serial),
      m_hallCalibrationRaw(), m_hallCalibrationRawRunStarted(false),
      m_hallCalibrationRawDurationMs(0UL),
      m_compactCalibrationResult(), m_hasCompactCalibrationResult(false),
      m_line(), m_length(0U), m_pendingJob(),
      m_hasPendingJob(false), m_waitingJobAck(false),
      m_lastJobSendMs(0UL), m_jobSendAttempts(0U),
      m_lastQueuedJobId(0UL), m_hasLastQueuedJobId(false),
      m_recoveryJobId(0UL), m_hasRecoveryJobId(false),
      m_jobDelivery(), m_hasJobDelivery(false),
      m_cancelJobId(0UL), m_hasPendingCancel(false),
      m_lastCancelSendMs(0UL), m_cancelSendAttempts(0U),
      m_jobCancel(), m_hasJobCancel(false)
{
}

void UartEventReceiver::begin(uint32_t baud, int8_t rxPin, int8_t txPin)
{
    m_serial.begin(baud, SERIAL_8N1, rxPin, txPin);
}

bool UartEventReceiver::poll(RemoteWindingEvent& event)
{
    if (m_hasJobDelivery || m_hasJobCancel) return false;

    while (m_serial.available() > 0)
    {
        const char value = static_cast<char>(m_serial.read());
        if (value == '\r') continue;
        if (value == '\n')
        {
            m_line[m_length] = '\0';
            bool eventReady = false;
            bool orderingBarrier = false;
            if (m_length > 0U)
            {
                if (strncmp(m_line, "CMP1|JOB_ACK|", 13U) == 0)
                {
                    processJobAck(m_line);
                    orderingBarrier = true;
                }
                else if (strncmp(m_line, "CMP1|JOB_CANCEL_ACK|", 20U) == 0)
                {
                    processCancelAck(m_line);
                    orderingBarrier = true;
                }
                else if (strncmp(m_line, "CMP1|CAL_SAMPLE|", 16U) == 0)
                {
                    processHallCalibrationRawSample(m_line);
                }
                else if (strncmp(m_line, "CMP1|CAL_DONE|", 14U) == 0)
                {
                    processHallCalibrationDone(m_line, millis());
                }
                else if (m_hardwareControl.processLine(m_line, millis()))
                {
                }
                else
                    eventReady = parseEventLine(m_line, event);
            }
            m_length = 0U;
            if (eventReady) return true;
            if (orderingBarrier) return false;
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
    if (m_hasPendingJob)
    {
        if (!m_waitingJobAck)
            sendPendingJob(nowMs);
        else if (static_cast<uint32_t>(nowMs - m_lastJobSendMs) >= JobRetryIntervalMs)
        {
            if (m_jobSendAttempts >= MaxJobSendAttempts)
            {
                publishJobDelivery(JobDeliveryResult::TimedOut,
                                   m_pendingJob.jobId,
                                   m_jobSendAttempts,
                                   "NO_ACK");
                m_hasPendingJob = false;
                m_waitingJobAck = false;
                m_jobSendAttempts = 0U;
            }
            else
                sendPendingJob(nowMs);
        }
    }

    if (m_hasPendingCancel &&
        (m_cancelSendAttempts == 0U ||
         static_cast<uint32_t>(nowMs - m_lastCancelSendMs) >= JobRetryIntervalMs))
    {
        if (m_cancelSendAttempts >= MaxCancelSendAttempts)
        {
            publishJobCancel(JobCancelResult::TimedOut,
                             m_cancelJobId,
                             m_cancelSendAttempts,
                             "NO_CANCEL_ACK");
            m_hasPendingCancel = false;
            m_cancelJobId = 0UL;
            m_cancelSendAttempts = 0U;
        }
        else
            sendPendingCancel(nowMs);
    }

    m_hardwareControl.update(nowMs);
}

bool UartEventReceiver::queueJob(const OutgoingWindingJob& job)
{
    if (m_hasPendingJob || m_hasJobDelivery || m_hasPendingCancel ||
        m_hasJobCancel || m_hardwareControl.requestPending() || !job.isValid())
        return false;
    if (m_hasLastQueuedJobId && job.jobId <= m_lastQueuedJobId) return false;
    m_pendingJob = job;
    m_hasPendingJob = true;
    m_waitingJobAck = false;
    m_jobSendAttempts = 0U;
    m_lastQueuedJobId = job.jobId;
    m_hasLastQueuedJobId = true;
    m_recoveryJobId = 0UL;
    m_hasRecoveryJobId = false;
    return true;
}

bool UartEventReceiver::cancelPendingJob(const char* detail)
{
    if (!m_hasPendingJob || m_hasJobDelivery || m_hasPendingCancel ||
        m_hasJobCancel || !isValidJobAckDetail(detail))
        return false;

    const uint32_t jobId = m_pendingJob.jobId;
    const uint8_t sendAttempts = m_jobSendAttempts;
    const bool mayHaveReachedArduino = m_waitingJobAck || sendAttempts != 0U;

    m_pendingJob = OutgoingWindingJob();
    m_hasPendingJob = false;
    m_waitingJobAck = false;
    m_lastJobSendMs = 0UL;
    m_jobSendAttempts = 0U;

    if (!mayHaveReachedArduino)
    {
        publishJobDelivery(JobDeliveryResult::Cancelled,
                           jobId,
                           0U,
                           detail != nullptr ? detail : "CANCELLED");
        return true;
    }

    m_cancelJobId = jobId;
    m_hasPendingCancel = true;
    m_lastCancelSendMs = 0UL;
    m_cancelSendAttempts = 0U;
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

bool UartEventReceiver::jobPending() const
{
    return m_hasPendingJob;
}

bool UartEventReceiver::requestJobCancel(uint32_t jobId)
{
    if (jobId == 0UL || m_hasPendingJob || m_hasJobDelivery ||
        m_hasPendingCancel || m_hasJobCancel || m_hardwareControl.requestPending())
        return false;
    m_cancelJobId = jobId;
    m_hasPendingCancel = true;
    m_lastCancelSendMs = 0UL;
    m_cancelSendAttempts = 0U;
    return true;
}

bool UartEventReceiver::takeJobCancel(JobCancelEvent& event)
{
    if (!m_hasJobCancel) return false;
    event = m_jobCancel;
    m_jobCancel = JobCancelEvent();
    m_hasJobCancel = false;
    return true;
}

bool UartEventReceiver::jobCancelPending() const
{
    return m_hasPendingCancel;
}

bool UartEventReceiver::controlLaneBusy() const
{
    return m_hasPendingJob || m_waitingJobAck || m_hasPendingCancel;
}

bool UartEventReceiver::requestHallSettings()
{
    return !controlLaneBusy() && m_hardwareControl.requestSettings();
}

bool UartEventReceiver::setHallSettings(
    uint16_t threshold,
    uint16_t hysteresis,
    uint16_t releaseDebounceMs,
    HallSignalDirectionRemote direction)
{
    return !controlLaneBusy() &&
           m_hardwareControl.setSettings(
               threshold, hysteresis, releaseDebounceMs, direction);
}

bool UartEventReceiver::resetHallSettings()
{
    return !controlLaneBusy() && m_hardwareControl.resetSettings();
}

bool UartEventReceiver::setHallTelemetryEnabled(bool enabled)
{
    return !controlLaneBusy() &&
           m_hardwareControl.setTelemetryEnabled(enabled);
}

bool UartEventReceiver::armHallCalibration()
{
    if (controlLaneBusy()) return false;
    m_hallCalibrationRaw.reset();
    m_hallCalibrationRawRunStarted = false;
    m_hallCalibrationRawDurationMs = 0UL;
    m_compactCalibrationResult = HallCalibrationRemoteResult();
    m_hasCompactCalibrationResult = false;
    return m_hardwareControl.armHallCalibration();
}

bool UartEventReceiver::abortHallCalibration()
{
    return !controlLaneBusy() && m_hardwareControl.abortHallCalibration();
}

bool UartEventReceiver::requestHallCalibration()
{
    return !controlLaneBusy() && m_hardwareControl.requestHallCalibration();
}

bool UartEventReceiver::proposeHallCalibration(
    uint32_t measurementId,
    uint16_t threshold,
    uint16_t hysteresis,
    uint16_t releaseDebounceMs,
    HallSignalDirectionRemote direction)
{
    return !controlLaneBusy() &&
           m_hardwareControl.proposeHallCalibration(
               measurementId, threshold, hysteresis, releaseDebounceMs, direction);
}

bool UartEventReceiver::hallControlPending() const
{
    return m_hardwareControl.requestPending();
}

bool UartEventReceiver::takeHallSettings(HallSettingsState& state)
{
    return m_hardwareControl.takeSettings(state);
}

bool UartEventReceiver::takeHallTelemetry(HallTelemetryState& state)
{
    return m_hardwareControl.takeTelemetry(state);
}

bool UartEventReceiver::takeHallCalibrationState(
    HallCalibrationRemoteStateSnapshot& state)
{
    return m_hardwareControl.takeHallCalibrationState(state);
}

bool UartEventReceiver::takeHallCalibrationResult(
    HallCalibrationRemoteResult& result)
{
    if (m_hasCompactCalibrationResult)
    {
        result = m_compactCalibrationResult;
        m_compactCalibrationResult = HallCalibrationRemoteResult();
        m_hasCompactCalibrationResult = false;
        return true;
    }

    if (!m_hardwareControl.takeHallCalibrationResult(result)) return false;
    HallCalibrationCompletionAdapter::enrichLegacy(
        result,
        m_hallCalibrationRaw,
        m_hallCalibrationRawRunStarted,
        m_hallCalibrationRawDurationMs);
    return true;
}

bool UartEventReceiver::takeHardwareControlReply(HardwareControlReply& reply)
{
    return m_hardwareControl.takeReply(reply);
}

void UartEventReceiver::rememberJobId(uint32_t jobId)
{
    if (jobId == 0UL) return;
    if (!m_hasLastQueuedJobId || jobId > m_lastQueuedJobId)
    {
        m_lastQueuedJobId = jobId;
        m_hasLastQueuedJobId = true;
    }
    m_recoveryJobId = jobId;
    m_hasRecoveryJobId = true;
}

void UartEventReceiver::sendAck(uint32_t runId, const char* status)
{
    writeRunReply("ACK", runId, status != nullptr ? status : "RECEIVED");
}

void UartEventReceiver::sendNack(uint32_t runId, const char* reason)
{
    writeRunReply("NACK", runId, reason != nullptr ? reason : "ERROR");
}

void UartEventReceiver::writeRunReply(const char* category,
                                      uint32_t runId,
                                      const char* detail)
{
    if (category == nullptr ||
        (strcmp(category, "ACK") != 0 && strcmp(category, "NACK") != 0) ||
        !isValidRunReplyDetail(detail)) return;

    char frame[80];
    const int payloadLength = snprintf(
        frame, sizeof(frame), "CMP1|%s|%lu|%s", category,
        static_cast<unsigned long>(runId), detail);
    if (payloadLength <= 0 ||
        static_cast<size_t>(payloadLength) >= sizeof(frame)) return;

    const uint16_t crc = Cmp1Crc::calculate(
        frame, static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf(
        frame + payloadLength,
        sizeof(frame) - static_cast<size_t>(payloadLength),
        "|%04X\n", static_cast<unsigned int>(crc));
    if (suffixLength <= 0 ||
        static_cast<size_t>(suffixLength) >=
            sizeof(frame) - static_cast<size_t>(payloadLength)) return;

    const size_t frameLength = static_cast<size_t>(payloadLength + suffixLength);
    m_serial.write(reinterpret_cast<const uint8_t*>(frame), frameLength);
}

bool UartEventReceiver::parseEventLine(char* line,
                                       RemoteWindingEvent& event) const
{
    event = RemoteWindingEvent();
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;
    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;
    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(line, payloadLength) != receivedCrc) return false;
    *lastSeparator = '\0';

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* typeText = strtok_r(nullptr, "|", &save);
    char* sessionText = strtok_r(nullptr, "|", &save);
    char* runText = strtok_r(nullptr, "|", &save);
    char* completedText = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || typeText == nullptr ||
        sessionText == nullptr || runText == nullptr || completedText == nullptr ||
        strcmp(version, "CMP1") != 0)
        return false;

    RemoteEventType parsedType = RemoteEventType::None;
    uint32_t parsedSession = 0UL;
    uint32_t parsedRun = 0UL;
    uint16_t parsedCompleted = 0U;
    if (!parseEventType(typeText, parsedType) ||
        !parseDecimal32(sessionText, parsedSession) || parsedSession == 0UL ||
        !parseDecimal32(runText, parsedRun) || parsedRun == 0UL ||
        !parseDecimal16(completedText, parsedCompleted) ||
        (parsedType == RemoteEventType::RunStarted && parsedCompleted != 0U) ||
        (parsedType == RemoteEventType::RunCompleted && parsedCompleted == 0U))
    {
        return false;
    }

    event.type = parsedType;
    event.sessionId = parsedSession;
    event.runId = parsedRun;
    event.completedRuns = parsedCompleted;

    if (strcmp(category, "EVT") == 0)
    {
        return strtok_r(nullptr, "|", &save) == nullptr;
    }
    if (strcmp(category, "LOCAL_EVT") != 0) return false;

    char* jobTypeText = strtok_r(nullptr, "|", &save);
    char* coilCountText = strtok_r(nullptr, "|", &save);
    char* turnsText = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (jobTypeText == nullptr || coilCountText == nullptr || turnsText == nullptr ||
        extra != nullptr)
        return false;

    if (strcmp(jobTypeText, "WORKING") == 0)
        event.jobType = RemoteJobType::Working;
    else if (strcmp(jobTypeText, "STARTING") == 0)
        event.jobType = RemoteJobType::Starting;
    else
        return false;

    uint32_t parsedCoilCount = 0UL;
    if (!parseDecimal32(coilCountText, parsedCoilCount) ||
        parsedCoilCount == 0UL || parsedCoilCount > RemoteWindingEvent::MaxCoils)
        return false;
    event.coilCount = static_cast<uint8_t>(parsedCoilCount);

    char* turnsSave = nullptr;
    char* token = strtok_r(turnsText, ",", &turnsSave);
    uint8_t index = 0U;
    while (token != nullptr && index < event.coilCount)
    {
        uint16_t value = 0U;
        if (!parseDecimal16(token, value) || value == 0U || value > 9999U)
            return false;
        event.turns[index++] = value;
        token = strtok_r(nullptr, ",", &turnsSave);
    }
    if (index != event.coilCount || token != nullptr) return false;
    event.localStandalone = true;
    return event.hasProgram();
}

bool UartEventReceiver::processJobAck(char* line)
{
    if (!validateAndStripJobReplyCrc(line)) return false;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* jobText = strtok_r(nullptr, "|", &save);
    char* status = strtok_r(nullptr, "|", &save);
    char* detail = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || jobText == nullptr ||
        status == nullptr || extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB_ACK") != 0 || !m_hasPendingJob ||
        !isValidJobAckDetail(detail) ||
        (capability != nullptr && strcmp(capability, "C") != 0))
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

bool UartEventReceiver::processCancelAck(char* line)
{
    if (!validateAndStripJobReplyCrc(line)) return false;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* jobText = strtok_r(nullptr, "|", &save);
    char* status = strtok_r(nullptr, "|", &save);
    char* detail = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || jobText == nullptr ||
        status == nullptr || extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "JOB_CANCEL_ACK") != 0 ||
        !isValidJobAckDetail(detail) ||
        (capability != nullptr && strcmp(capability, "C") != 0))
        return false;

    uint32_t jobId = 0UL;
    if (!parseDecimal32(jobText, jobId)) return false;

    if (jobId == 0UL)
    {
        if (strcmp(status, "CANCELLED") != 0 || detail == nullptr ||
            strcmp(detail, "ALL_CLEAR") != 0)
            return false;

        const uint32_t clearedJobId = m_hasPendingCancel
            ? m_cancelJobId
            : (m_hasRecoveryJobId ? m_recoveryJobId : 0UL);
        if (clearedJobId == 0UL) return false;

        m_pendingJob = OutgoingWindingJob();
        m_hasPendingJob = false;
        m_waitingJobAck = false;
        m_lastJobSendMs = 0UL;
        m_jobSendAttempts = 0U;
        m_hasPendingCancel = false;
        m_cancelJobId = 0UL;
        m_lastCancelSendMs = 0UL;
        m_cancelSendAttempts = 0U;
        m_recoveryJobId = 0UL;
        m_hasRecoveryJobId = false;
        publishJobCancel(JobCancelResult::Cancelled,
                         clearedJobId,
                         0U,
                         "ALL_CLEAR");
        return true;
    }

    if (!m_hasPendingCancel || jobId != m_cancelJobId) return false;

    JobCancelResult result = JobCancelResult::None;
    if (strcmp(status, "CANCELLED") == 0)
        result = JobCancelResult::Cancelled;
    else if (strcmp(status, "REJECTED") == 0)
    {
        if (detail == nullptr) return false;
        result = JobCancelResult::Rejected;
    }
    else
        return false;

    publishJobCancel(result,
                     jobId,
                     m_cancelSendAttempts,
                     detail != nullptr ? detail :
                     (result == JobCancelResult::Cancelled ? "CANCELLED" : "REJECTED"));
    m_hasPendingCancel = false;
    m_cancelJobId = 0UL;
    m_cancelSendAttempts = 0U;
    if (m_hasRecoveryJobId && jobId == m_recoveryJobId)
    {
        m_recoveryJobId = 0UL;
        m_hasRecoveryJobId = false;
    }
    return true;
}

bool UartEventReceiver::processHallCalibrationRawSample(char* line)
{
    HallCalibrationRawSample sample;
    if (!HallCalibrationRawProtocol::parseSample(line, sample) || !sample.valid)
        return false;

    if (sample.phase == HallCalibrationRawPhase::Baseline)
    {
        if (m_hallCalibrationRawRunStarted) return false;
        return m_hallCalibrationRaw.addBaselineSample(sample.rawAdc);
    }

    if (!m_hallCalibrationRawRunStarted)
    {
        m_hallCalibrationRaw.beginRun();
        m_hallCalibrationRawRunStarted = true;
    }

    if (!m_hallCalibrationRaw.addRunSample(sample.rawAdc)) return false;
    m_hallCalibrationRawDurationMs = sample.elapsedMs;
    return true;
}

bool UartEventReceiver::processHallCalibrationDone(char* line, uint32_t nowMs)
{
    HallCalibrationDone done;
    if (!HallCalibrationDoneProtocol::parseDone(line, done) || !done.valid ||
        m_hasCompactCalibrationResult)
        return false;

    HallCalibrationRemoteResult result;
    if (!HallCalibrationCompletionAdapter::buildFromDone(
            done,
            m_hallCalibrationRaw,
            m_hallCalibrationRawRunStarted,
            m_hallCalibrationRawDurationMs,
            nowMs,
            result))
        return false;

    m_compactCalibrationResult = result;
    m_hasCompactCalibrationResult = true;
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
        payload, sizeof(payload), "CMP1|JOB|%lu|%lu|%s|%u|%s|R%u|C",
        static_cast<unsigned long>(job.jobId),
        static_cast<unsigned long>(job.sessionId),
        job.type == RemoteJobType::Starting ? "STARTING" : "WORKING",
        static_cast<unsigned int>(job.coilCount), turnsText,
        static_cast<unsigned int>(job.repeatTarget));
    if (payloadLength <= 0 || static_cast<size_t>(payloadLength) >= sizeof(payload))
        return false;

    const uint16_t crc = Cmp1Crc::calculate(
        payload, static_cast<size_t>(payloadLength));
    m_serial.print(payload);
    m_serial.print('|');
    if (crc < 0x1000U) m_serial.print('0');
    if (crc < 0x0100U) m_serial.print('0');
    if (crc < 0x0010U) m_serial.print('0');
    m_serial.println(crc, HEX);
    return true;
}

bool UartEventReceiver::sendPendingCancel(uint32_t nowMs)
{
    if (!m_hasPendingCancel || m_cancelJobId == 0UL ||
        m_cancelSendAttempts >= MaxCancelSendAttempts ||
        !writeCancelFrame(m_cancelJobId))
        return false;
    ++m_cancelSendAttempts;
    m_lastCancelSendMs = nowMs;
    return true;
}

bool UartEventReceiver::writeCancelFrame(uint32_t jobId)
{
    if (jobId == 0UL) return false;
    char payload[48];
    const int payloadLength = snprintf(payload, sizeof(payload),
                                       "CMP1|JOB_CANCEL|%lu",
                                       static_cast<unsigned long>(jobId));
    if (payloadLength <= 0 || static_cast<size_t>(payloadLength) >= sizeof(payload))
        return false;

    const uint16_t crc = Cmp1Crc::calculate(
        payload, static_cast<size_t>(payloadLength));
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

void UartEventReceiver::publishJobCancel(JobCancelResult result,
                                         uint32_t jobId,
                                         uint8_t sendAttempts,
                                         const char* detail)
{
    m_jobCancel.result = result;
    m_jobCancel.jobId = jobId;
    m_jobCancel.sendAttempts = sendAttempts;
    const char* source = detail != nullptr ? detail : "";
    strncpy(m_jobCancel.detail, source, JobDeliveryEvent::MaxDetailLength);
    m_jobCancel.detail[JobDeliveryEvent::MaxDetailLength] = '\0';
    m_hasJobCancel = true;
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

bool UartEventReceiver::validateAndStripJobReplyCrc(char* line)
{
    if (line == nullptr) return false;
    uint8_t separatorCount = 0U;
    for (const char* value = line; *value != '\0'; ++value)
        if (*value == '|') ++separatorCount;

    if (separatorCount == 4U) return true;
    if (separatorCount != 6U) return false;

    char* lastSeparator = strrchr(line, '|');
    uint16_t receivedCrc = 0U;
    if (lastSeparator == nullptr ||
        !parseHex16(lastSeparator + 1, receivedCrc)) return false;
    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(line, payloadLength) != receivedCrc) return false;
    *lastSeparator = '\0';
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
