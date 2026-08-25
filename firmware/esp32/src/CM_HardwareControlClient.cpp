#include "CM_HardwareControlClient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{

HallSettingsState::HallSettingsState()
    : threshold(0U), hysteresis(0U), releaseDebounceMs(0U),
      direction(HallSignalDirectionRemote::Rising), fromEeprom(false),
      valid(false), receivedAtMs(0UL)
{
}

HallTelemetryState::HallTelemetryState()
    : rawAdc(0U), windowMin(0U), windowMax(0U), threshold(0U),
      hysteresis(0U), releaseBoundary(0U), releaseDebounceMs(0U),
      direction(HallSignalDirectionRemote::Rising), magnetDetected(false),
      rearmState(), sampleCount(0U), capturedAtMs(0UL), receivedAtMs(0UL),
      valid(false)
{
    rearmState[0] = '\0';
}

HallCalibrationRemoteStateSnapshot::HallCalibrationRemoteStateSnapshot()
    : state(HallCalibrationRemoteState::Idle), baselineReady(false),
      motorPermit(false), valid(false), receivedAtMs(0UL)
{
}

HallCalibrationRemoteResult::HallCalibrationRemoteResult()
    : recommendationValid(false), measurementId(0UL), baselineAdc(0U),
      minAdc(0U), maxAdc(0U), recommendedThreshold(0U),
      recommendedHysteresis(0U), direction(HallSignalDirectionRemote::Rising),
      sampleCount(0U), durationMs(0UL), valid(false), receivedAtMs(0UL)
{
}

HardwareControlReply::HardwareControlReply()
    : result(HardwareControlReplyResult::None), sendAttempts(0U)
{
}

HardwareControlClient::HardwareControlClient(HardwareSerial& serial)
    : m_serial(serial), m_requestType(RequestType::None), m_requestPayload(),
      m_waitingReply(false), m_lastSendMs(0UL), m_sendAttempts(0U),
      m_pendingCalibrationMeasurementId(0UL),
      m_settings(), m_hasSettings(false), m_telemetry(), m_hasTelemetry(false),
      m_calibrationState(), m_hasCalibrationState(false),
      m_lastCalibrationKeepAliveMs(0UL),
      m_calibrationResult(), m_hasCalibrationResult(false),
      m_reply(), m_hasReply(false)
{
    m_requestPayload[0] = '\0';
}

void HardwareControlClient::update(uint32_t nowMs)
{
    if (m_requestType == RequestType::StageCalibrationProposal &&
        m_calibrationState.valid &&
        m_calibrationState.state == HallCalibrationRemoteState::WaitingApplyConfirm)
    {
        if (static_cast<uint32_t>(nowMs - m_lastCalibrationKeepAliveMs) >=
            CalibrationKeepAliveMs)
        {
            sendCalibrationKeepAlive(nowMs);
        }
        return;
    }

    if (m_requestType != RequestType::None)
    {
        if (!m_waitingReply)
        {
            sendPending(nowMs);
            return;
        }

        if (static_cast<uint32_t>(nowMs - m_lastSendMs) < RetryIntervalMs) return;

        if (m_sendAttempts >= MaxSendAttempts)
        {
            finishRequest(HardwareControlReplyResult::TimedOut);
            return;
        }

        sendPending(nowMs);
        return;
    }

    if (m_calibrationState.valid && calibrationActive(m_calibrationState.state) &&
        static_cast<uint32_t>(nowMs - m_lastCalibrationKeepAliveMs) >=
            CalibrationKeepAliveMs)
    {
        sendCalibrationKeepAlive(nowMs);
    }
}

bool HardwareControlClient::processLine(char* line, uint32_t nowMs)
{
    if (line == nullptr) return false;

    if (strncmp(line, "CMP1|CFG_STATE|", 15U) == 0)
        return processSettingsState(line, nowMs);
    if (strncmp(line, "CMP1|CFG_ACK|", 13U) == 0 ||
        strncmp(line, "CMP1|CFG_NACK|", 14U) == 0)
        return processSettingsResult(line);
    if (strncmp(line, "CMP1|HALL_STATE|", 16U) == 0)
        return processTelemetryState(line, nowMs);
    if (strncmp(line, "CMP1|CAL_STATE|", 15U) == 0)
        return processCalibrationState(line, nowMs);
    if (strncmp(line, "CMP1|CAL_RESULT|", 16U) == 0)
        return processCalibrationResult(line, nowMs);
    if (strncmp(line, "CMP1|CAL_APPLIED|", 17U) == 0)
        return processCalibrationApplied(line, nowMs);

    return false;
}

bool HardwareControlClient::requestSettings()
{
    return queueRequest(RequestType::GetSettings, "CMP1|CFG_GET|HALL|C");
}

bool HardwareControlClient::setSettings(uint16_t threshold,
                                        uint16_t hysteresis,
                                        uint16_t releaseDebounceMs,
                                        HallSignalDirectionRemote direction)
{
    if (threshold == 0U || threshold > 1023U ||
        hysteresis == 0U || hysteresis > 512U || hysteresis >= threshold ||
        releaseDebounceMs == 0U || releaseDebounceMs > 1000U)
    {
        return false;
    }

    char payload[MaxRequestPayloadLength];
    const int length = snprintf(
        payload, sizeof(payload), "CMP1|CFG_SET|HALL|%u|%u|%u|%s|C",
        static_cast<unsigned int>(threshold),
        static_cast<unsigned int>(hysteresis),
        static_cast<unsigned int>(releaseDebounceMs),
        direction == HallSignalDirectionRemote::Falling ? "FALLING" : "RISING");
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(payload)) return false;
    return queueRequest(RequestType::SetSettings, payload);
}

bool HardwareControlClient::resetSettings()
{
    return queueRequest(RequestType::ResetSettings, "CMP1|CFG_RESET|HALL|C");
}

bool HardwareControlClient::setTelemetryEnabled(bool enabled)
{
    return queueRequest(enabled ? RequestType::StartTelemetry
                                : RequestType::StopTelemetry,
                        enabled ? "CMP1|HALL_TELEM|START|C"
                                : "CMP1|HALL_TELEM|STOP|C");
}

bool HardwareControlClient::armHallCalibration()
{
    return queueRequest(RequestType::ArmCalibration, "CMP1|CAL|ARM|C");
}

bool HardwareControlClient::abortHallCalibration()
{
    if (m_requestType == RequestType::StageCalibrationProposal &&
        m_calibrationState.valid &&
        m_calibrationState.state == HallCalibrationRemoteState::WaitingApplyConfirm)
    {
        m_requestType = RequestType::None;
        m_requestPayload[0] = '\0';
        m_waitingReply = false;
        m_lastSendMs = 0UL;
        m_sendAttempts = 0U;
        m_pendingCalibrationMeasurementId = 0UL;
    }
    return queueRequest(RequestType::AbortCalibration, "CMP1|CAL|ABORT|C");
}

bool HardwareControlClient::requestHallCalibration()
{
    return queueRequest(RequestType::GetCalibration, "CMP1|CAL|GET|C");
}

bool HardwareControlClient::proposeHallCalibration(
    uint32_t measurementId,
    uint16_t threshold,
    uint16_t hysteresis,
    uint16_t releaseDebounceMs,
    HallSignalDirectionRemote direction)
{
    if (measurementId == 0UL || threshold == 0U || threshold > 1023U ||
        hysteresis == 0U || hysteresis > 512U || hysteresis >= threshold ||
        releaseDebounceMs == 0U || releaseDebounceMs > 1000U)
    {
        return false;
    }

    char payload[MaxRequestPayloadLength];
    const int length = snprintf(
        payload, sizeof(payload),
        "CMP1|CAL_PROPOSAL|%lu|%u|%u|%u|%s|C",
        static_cast<unsigned long>(measurementId),
        static_cast<unsigned int>(threshold),
        static_cast<unsigned int>(hysteresis),
        static_cast<unsigned int>(releaseDebounceMs),
        direction == HallSignalDirectionRemote::Falling ? "FALLING" : "RISING");
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(payload)) return false;
    if (!queueRequest(RequestType::StageCalibrationProposal, payload)) return false;
    m_pendingCalibrationMeasurementId = measurementId;
    return true;
}

bool HardwareControlClient::requestPending() const
{
    return m_requestType != RequestType::None;
}

bool HardwareControlClient::takeSettings(HallSettingsState& state)
{
    if (!m_hasSettings) return false;
    state = m_settings;
    m_hasSettings = false;
    return true;
}

bool HardwareControlClient::takeTelemetry(HallTelemetryState& state)
{
    if (!m_hasTelemetry) return false;
    state = m_telemetry;
    m_hasTelemetry = false;
    return true;
}

bool HardwareControlClient::takeHallCalibrationState(
    HallCalibrationRemoteStateSnapshot& state)
{
    if (!m_hasCalibrationState) return false;
    state = m_calibrationState;
    m_hasCalibrationState = false;
    return true;
}

bool HardwareControlClient::takeHallCalibrationResult(
    HallCalibrationRemoteResult& result)
{
    if (!m_hasCalibrationResult) return false;
    result = m_calibrationResult;
    m_hasCalibrationResult = false;
    return true;
}

bool HardwareControlClient::takeReply(HardwareControlReply& reply)
{
    if (!m_hasReply) return false;
    reply = m_reply;
    m_reply = HardwareControlReply();
    m_hasReply = false;
    return true;
}

bool HardwareControlClient::queueRequest(RequestType type, const char* payload)
{
    if (type == RequestType::None || payload == nullptr ||
        m_requestType != RequestType::None || m_hasReply)
        return false;

    const size_t length = strlen(payload);
    if (length == 0U || length >= sizeof(m_requestPayload)) return false;

    memcpy(m_requestPayload, payload, length + 1U);
    m_requestType = type;
    m_waitingReply = false;
    m_lastSendMs = 0UL;
    m_sendAttempts = 0U;
    return true;
}

bool HardwareControlClient::sendPending(uint32_t nowMs)
{
    if (m_requestType == RequestType::None || m_requestPayload[0] == '\0' ||
        m_sendAttempts >= MaxSendAttempts)
        return false;

    const size_t payloadLength = strlen(m_requestPayload);
    const uint16_t crc = Cmp1Crc::calculate(m_requestPayload, payloadLength);
    m_serial.print(m_requestPayload);
    m_serial.print('|');
    if (crc < 0x1000U) m_serial.print('0');
    if (crc < 0x0100U) m_serial.print('0');
    if (crc < 0x0010U) m_serial.print('0');
    m_serial.println(crc, HEX);

    ++m_sendAttempts;
    m_waitingReply = true;
    m_lastSendMs = nowMs;
    return true;
}

bool HardwareControlClient::sendCalibrationKeepAlive(uint32_t nowMs)
{
    static const char Payload[] = "CMP1|CAL|GET|C";
    const uint16_t crc = Cmp1Crc::calculate(Payload, sizeof(Payload) - 1U);
    m_serial.print(Payload);
    m_serial.print('|');
    if (crc < 0x1000U) m_serial.print('0');
    if (crc < 0x0100U) m_serial.print('0');
    if (crc < 0x0010U) m_serial.print('0');
    m_serial.println(crc, HEX);
    m_lastCalibrationKeepAliveMs = nowMs;
    return true;
}

bool HardwareControlClient::processSettingsState(char* line, uint32_t nowMs)
{
    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* target = strtok_r(nullptr, "|", &save);
    char* thresholdText = strtok_r(nullptr, "|", &save);
    char* hysteresisText = strtok_r(nullptr, "|", &save);
    char* debounceText = strtok_r(nullptr, "|", &save);
    char* directionText = strtok_r(nullptr, "|", &save);
    char* sourceText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || target == nullptr ||
        thresholdText == nullptr || hysteresisText == nullptr ||
        debounceText == nullptr || directionText == nullptr ||
        sourceText == nullptr || capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "CFG_STATE") != 0 ||
        strcmp(target, "HALL") != 0 || strcmp(capability, "C") != 0)
        return true;

    HallSettingsState parsed;
    if (!parseDecimal16(thresholdText, parsed.threshold) ||
        !parseDecimal16(hysteresisText, parsed.hysteresis) ||
        !parseDecimal16(debounceText, parsed.releaseDebounceMs))
        return true;

    if (parsed.threshold == 0U || parsed.threshold > 1023U ||
        parsed.hysteresis == 0U || parsed.hysteresis > 512U ||
        parsed.hysteresis >= parsed.threshold ||
        parsed.releaseDebounceMs == 0U || parsed.releaseDebounceMs > 1000U)
    {
        return true;
    }

    if (strcmp(directionText, "RISING") == 0)
        parsed.direction = HallSignalDirectionRemote::Rising;
    else if (strcmp(directionText, "FALLING") == 0)
        parsed.direction = HallSignalDirectionRemote::Falling;
    else
        return true;

    if (strcmp(sourceText, "EEPROM") == 0)
        parsed.fromEeprom = true;
    else if (strcmp(sourceText, "FACTORY") == 0)
        parsed.fromEeprom = false;
    else
        return true;

    parsed.valid = true;
    parsed.receivedAtMs = nowMs;
    m_settings = parsed;
    m_hasSettings = true;

    if (m_requestType == RequestType::GetSettings)
    {
        if (m_pendingCalibrationMeasurementId != 0UL)
        {
            finishRequest(HardwareControlReplyResult::TimedOut);
            return true;
        }
        m_requestType = RequestType::None;
        m_waitingReply = false;
        m_sendAttempts = 0U;
        m_requestPayload[0] = '\0';
    }
    return true;
}

bool HardwareControlClient::processSettingsResult(char* line)
{
    const bool ackPrefix = strncmp(line, "CMP1|CFG_ACK|", 13U) == 0;
    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* target = strtok_r(nullptr, "|", &save);
    char* resultText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);
    if (version == nullptr || category == nullptr || target == nullptr ||
        resultText == nullptr || capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(target, "HALL") != 0 ||
        strcmp(capability, "C") != 0)
        return true;

    const bool categoryAck = strcmp(category, "CFG_ACK") == 0;
    const bool categoryNack = strcmp(category, "CFG_NACK") == 0;
    if ((!categoryAck && !categoryNack) || categoryAck != ackPrefix) return true;

    HardwareControlReplyResult result = parseResultName(resultText);
    if (categoryAck && result != HardwareControlReplyResult::Applied) return true;
    if (categoryNack && result == HardwareControlReplyResult::Applied) return true;
    finishRequest(result);
    return true;
}

bool HardwareControlClient::processTelemetryState(char* line, uint32_t nowMs)
{
    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* rawText = strtok_r(nullptr, "|", &save);
    char* minText = strtok_r(nullptr, "|", &save);
    char* maxText = strtok_r(nullptr, "|", &save);
    char* thresholdText = strtok_r(nullptr, "|", &save);
    char* hysteresisText = strtok_r(nullptr, "|", &save);
    char* releaseText = strtok_r(nullptr, "|", &save);
    char* debounceText = strtok_r(nullptr, "|", &save);
    char* directionText = strtok_r(nullptr, "|", &save);
    char* magnetText = strtok_r(nullptr, "|", &save);
    char* rearmText = strtok_r(nullptr, "|", &save);
    char* samplesText = strtok_r(nullptr, "|", &save);
    char* capturedText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || rawText == nullptr ||
        minText == nullptr || maxText == nullptr || thresholdText == nullptr ||
        hysteresisText == nullptr || releaseText == nullptr || debounceText == nullptr ||
        directionText == nullptr || magnetText == nullptr || rearmText == nullptr ||
        samplesText == nullptr || capturedText == nullptr || capability == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "HALL_STATE") != 0 || strcmp(capability, "C") != 0)
        return true;

    HallTelemetryState parsed;
    if (!parseDecimal16(rawText, parsed.rawAdc) ||
        !parseDecimal16(minText, parsed.windowMin) ||
        !parseDecimal16(maxText, parsed.windowMax) ||
        !parseDecimal16(thresholdText, parsed.threshold) ||
        !parseDecimal16(hysteresisText, parsed.hysteresis) ||
        !parseDecimal16(releaseText, parsed.releaseBoundary) ||
        !parseDecimal16(debounceText, parsed.releaseDebounceMs) ||
        !parseDecimal16(samplesText, parsed.sampleCount) ||
        !parseDecimal32(capturedText, parsed.capturedAtMs))
        return true;

    if (strcmp(directionText, "RISING") == 0)
        parsed.direction = HallSignalDirectionRemote::Rising;
    else if (strcmp(directionText, "FALLING") == 0)
        parsed.direction = HallSignalDirectionRemote::Falling;
    else
        return true;

    const uint16_t expectedReleaseBoundary =
        parsed.direction == HallSignalDirectionRemote::Falling
            ? static_cast<uint16_t>(
                  static_cast<uint32_t>(parsed.threshold) +
                          static_cast<uint32_t>(parsed.hysteresis) > 1023UL
                      ? 1023UL
                      : static_cast<uint32_t>(parsed.threshold) +
                            static_cast<uint32_t>(parsed.hysteresis))
            : static_cast<uint16_t>(parsed.threshold - parsed.hysteresis);

    if (parsed.rawAdc > 1023U || parsed.windowMin > 1023U ||
        parsed.windowMax > 1023U || parsed.windowMin > parsed.windowMax ||
        parsed.rawAdc < parsed.windowMin || parsed.rawAdc > parsed.windowMax ||
        parsed.threshold == 0U || parsed.threshold > 1023U ||
        parsed.hysteresis == 0U || parsed.hysteresis > 512U ||
        parsed.hysteresis >= parsed.threshold ||
        parsed.releaseBoundary != expectedReleaseBoundary ||
        parsed.releaseDebounceMs == 0U || parsed.releaseDebounceMs > 1000U ||
        parsed.sampleCount == 0U)
    {
        return true;
    }

    if (strcmp(magnetText, "0") == 0)
        parsed.magnetDetected = false;
    else if (strcmp(magnetText, "1") == 0)
        parsed.magnetDetected = true;
    else
        return true;

    if (strcmp(rearmText, "ARMED") != 0 &&
        strcmp(rearmText, "WAITING_RELEASE") != 0 &&
        strcmp(rearmText, "RELEASE_DEBOUNCE") != 0)
        return true;
    strncpy(parsed.rearmState, rearmText, sizeof(parsed.rearmState) - 1U);
    parsed.rearmState[sizeof(parsed.rearmState) - 1U] = '\0';

    parsed.receivedAtMs = nowMs;
    parsed.valid = true;
    m_telemetry = parsed;
    m_hasTelemetry = true;
    return true;
}

bool HardwareControlClient::processCalibrationState(char* line, uint32_t nowMs)
{
    const bool proposalWasWaitingApply =
        m_requestType == RequestType::StageCalibrationProposal &&
        m_calibrationState.valid &&
        m_calibrationState.state == HallCalibrationRemoteState::WaitingApplyConfirm;

    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* stateText = strtok_r(nullptr, "|", &save);
    char* baselineText = strtok_r(nullptr, "|", &save);
    char* permitText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || stateText == nullptr ||
        baselineText == nullptr || permitText == nullptr || capability == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "CAL_STATE") != 0 || strcmp(capability, "C") != 0)
        return true;

    HallCalibrationRemoteStateSnapshot parsed;
    if (strcmp(stateText, "IDLE") == 0)
        parsed.state = HallCalibrationRemoteState::Idle;
    else if (strcmp(stateText, "WAITING_LOCAL_CONFIRM") == 0)
        parsed.state = HallCalibrationRemoteState::WaitingLocalConfirm;
    else if (strcmp(stateText, "ARMED_WAITING_START") == 0)
        parsed.state = HallCalibrationRemoteState::ArmedWaitingPhysicalStart;
    else if (strcmp(stateText, "RUNNING") == 0)
        parsed.state = HallCalibrationRemoteState::Running;
    else if (strcmp(stateText, "COMPLETED") == 0)
        parsed.state = HallCalibrationRemoteState::Completed;
    else if (strcmp(stateText, "WAITING_APPLY_CONFIRM") == 0)
        parsed.state = HallCalibrationRemoteState::WaitingApplyConfirm;
    else if (strcmp(stateText, "ABORTED") == 0)
        parsed.state = HallCalibrationRemoteState::Aborted;
    else
        return true;

    if (strcmp(baselineText, "0") == 0)
        parsed.baselineReady = false;
    else if (strcmp(baselineText, "1") == 0)
        parsed.baselineReady = true;
    else
        return true;

    if (strcmp(permitText, "0") == 0)
        parsed.motorPermit = false;
    else if (strcmp(permitText, "1") == 0)
        parsed.motorPermit = true;
    else
        return true;

    if (parsed.motorPermit && parsed.state != HallCalibrationRemoteState::Running)
        return true;

    parsed.valid = true;
    parsed.receivedAtMs = nowMs;
    m_calibrationState = parsed;
    m_hasCalibrationState = true;
    m_lastCalibrationKeepAliveMs = calibrationActive(parsed.state) ? nowMs : 0UL;

    if (proposalWasWaitingApply)
    {
        if (parsed.state == HallCalibrationRemoteState::Aborted)
        {
            finishRequest(HardwareControlReplyResult::Cancelled);
            return true;
        }
        if (parsed.state == HallCalibrationRemoteState::Completed)
        {
            static const char ReconcilePayload[] = "CMP1|CFG_GET|HALL|C";
            memcpy(m_requestPayload, ReconcilePayload, sizeof(ReconcilePayload));
            m_requestType = RequestType::GetSettings;
            m_waitingReply = false;
            m_lastSendMs = 0UL;
            m_sendAttempts = 0U;
            return true;
        }
    }

    if (m_requestType == RequestType::ArmCalibration)
    {
        finishRequest(parsed.state == HallCalibrationRemoteState::WaitingLocalConfirm
                          ? HardwareControlReplyResult::Applied
                          : HardwareControlReplyResult::Busy);
    }
    else if (m_requestType == RequestType::AbortCalibration)
    {
        const bool stopped = parsed.state == HallCalibrationRemoteState::Aborted ||
                             parsed.state == HallCalibrationRemoteState::Idle ||
                             parsed.state == HallCalibrationRemoteState::Completed;
        finishRequest(stopped ? HardwareControlReplyResult::Applied
                              : HardwareControlReplyResult::Busy);
    }
    else if (m_requestType == RequestType::GetCalibration)
    {
        finishRequest(HardwareControlReplyResult::Applied);
    }

    return true;
}

bool HardwareControlClient::processCalibrationResult(char* line, uint32_t nowMs)
{
    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* validityText = strtok_r(nullptr, "|", &save);
    char* baselineText = strtok_r(nullptr, "|", &save);
    char* minText = strtok_r(nullptr, "|", &save);
    char* maxText = strtok_r(nullptr, "|", &save);
    char* thresholdText = strtok_r(nullptr, "|", &save);
    char* hysteresisText = strtok_r(nullptr, "|", &save);
    char* directionText = strtok_r(nullptr, "|", &save);
    char* samplesText = strtok_r(nullptr, "|", &save);
    char* durationText = strtok_r(nullptr, "|", &save);
    char* measurementText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || validityText == nullptr ||
        baselineText == nullptr || minText == nullptr || maxText == nullptr ||
        thresholdText == nullptr || hysteresisText == nullptr ||
        directionText == nullptr || samplesText == nullptr || durationText == nullptr ||
        measurementText == nullptr || capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "CAL_RESULT") != 0 ||
        strcmp(capability, "C") != 0)
        return true;

    HallCalibrationRemoteResult parsed;
    if (strcmp(validityText, "VALID") == 0)
        parsed.recommendationValid = true;
    else if (strcmp(validityText, "INVALID") == 0)
        parsed.recommendationValid = false;
    else
        return true;

    if (!parseDecimal16(baselineText, parsed.baselineAdc) ||
        !parseDecimal16(minText, parsed.minAdc) ||
        !parseDecimal16(maxText, parsed.maxAdc) ||
        !parseDecimal16(thresholdText, parsed.recommendedThreshold) ||
        !parseDecimal16(hysteresisText, parsed.recommendedHysteresis) ||
        !parseDecimal16(samplesText, parsed.sampleCount) ||
        !parseDecimal32(durationText, parsed.durationMs) ||
        !parseDecimal32(measurementText, parsed.measurementId) ||
        parsed.measurementId == 0UL)
        return true;

    if (strcmp(directionText, "RISING") == 0)
        parsed.direction = HallSignalDirectionRemote::Rising;
    else if (strcmp(directionText, "FALLING") == 0)
        parsed.direction = HallSignalDirectionRemote::Falling;
    else
        return true;

    parsed.valid = true;
    parsed.receivedAtMs = nowMs;
    m_calibrationResult = parsed;
    m_hasCalibrationResult = true;
    return true;
}

bool HardwareControlClient::processCalibrationApplied(char* line, uint32_t nowMs)
{
    if (!validateAndStripCrc(line)) return true;

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* measurementText = strtok_r(nullptr, "|", &save);
    char* resultText = strtok_r(nullptr, "|", &save);
    char* thresholdText = strtok_r(nullptr, "|", &save);
    char* hysteresisText = strtok_r(nullptr, "|", &save);
    char* debounceText = strtok_r(nullptr, "|", &save);
    char* directionText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    uint32_t measurementId = 0UL;
    HallSettingsState applied;
    if (version == nullptr || category == nullptr || measurementText == nullptr ||
        resultText == nullptr || thresholdText == nullptr || hysteresisText == nullptr ||
        debounceText == nullptr || directionText == nullptr || capability == nullptr ||
        extra != nullptr || strcmp(version, "CMP1") != 0 ||
        strcmp(category, "CAL_APPLIED") != 0 || strcmp(capability, "C") != 0 ||
        !parseDecimal32(measurementText, measurementId) || measurementId == 0UL ||
        !parseDecimal16(thresholdText, applied.threshold) ||
        !parseDecimal16(hysteresisText, applied.hysteresis) ||
        !parseDecimal16(debounceText, applied.releaseDebounceMs))
    {
        return true;
    }

    if (strcmp(directionText, "RISING") == 0)
        applied.direction = HallSignalDirectionRemote::Rising;
    else if (strcmp(directionText, "FALLING") == 0)
        applied.direction = HallSignalDirectionRemote::Falling;
    else
        return true;

    if (applied.threshold == 0U || applied.threshold > 1023U ||
        applied.hysteresis == 0U || applied.hysteresis > 512U ||
        applied.hysteresis >= applied.threshold ||
        applied.releaseDebounceMs == 0U || applied.releaseDebounceMs > 1000U)
    {
        return true;
    }

    if (m_requestType != RequestType::StageCalibrationProposal ||
        measurementId != m_pendingCalibrationMeasurementId)
    {
        return true;
    }

    const HardwareControlReplyResult result = parseResultName(resultText);
    if (result == HardwareControlReplyResult::Applied)
    {
        applied.fromEeprom = true;
        applied.valid = true;
        applied.receivedAtMs = nowMs;
        m_settings = applied;
        m_hasSettings = true;
    }

    finishRequest(result);
    return true;
}

void HardwareControlClient::finishRequest(HardwareControlReplyResult result)
{
    m_reply.result = result;
    m_reply.sendAttempts = m_sendAttempts;
    m_hasReply = true;
    m_requestType = RequestType::None;
    m_requestPayload[0] = '\0';
    m_waitingReply = false;
    m_lastSendMs = 0UL;
    m_sendAttempts = 0U;
    m_pendingCalibrationMeasurementId = 0UL;
}

bool HardwareControlClient::validateAndStripCrc(char* line)
{
    if (line == nullptr) return false;
    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;
    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(line, payloadLength) != receivedCrc) return false;
    *lastSeparator = '\0';
    return true;
}

bool HardwareControlClient::parseDecimal16(const char* text, uint16_t& value)
{
    uint32_t parsed = 0UL;
    if (!parseDecimal32(text, parsed) || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool HardwareControlClient::parseDecimal32(const char* text, uint32_t& value)
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

bool HardwareControlClient::parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

HardwareControlReplyResult HardwareControlClient::parseResultName(const char* text)
{
    if (text == nullptr) return HardwareControlReplyResult::Unsupported;
    if (strcmp(text, "APPLIED") == 0) return HardwareControlReplyResult::Applied;
    if (strcmp(text, "BUSY") == 0) return HardwareControlReplyResult::Busy;
    if (strcmp(text, "INVALID") == 0) return HardwareControlReplyResult::Invalid;
    if (strcmp(text, "IDENTITY_MISMATCH") == 0)
        return HardwareControlReplyResult::IdentityMismatch;
    if (strcmp(text, "CANCELLED") == 0)
        return HardwareControlReplyResult::Cancelled;
    if (strcmp(text, "PERSISTENCE_FAILED") == 0)
        return HardwareControlReplyResult::PersistenceFailed;
    return HardwareControlReplyResult::Unsupported;
}

bool HardwareControlClient::calibrationActive(HallCalibrationRemoteState state)
{
    return state == HallCalibrationRemoteState::WaitingLocalConfirm ||
           state == HallCalibrationRemoteState::ArmedWaitingPhysicalStart ||
           state == HallCalibrationRemoteState::Running ||
           state == HallCalibrationRemoteState::WaitingApplyConfirm;
}

} // namespace CM
