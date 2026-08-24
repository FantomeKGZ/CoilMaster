#include "CM_HardwareControlWeb.h"
#include "CM_HallCalibrationAnalyzer.h"

namespace CM
{

HardwareControlWeb::HardwareControlWeb(WebServer& server,
                                       UartEventReceiver& receiver)
    : m_server(server),
      m_receiver(receiver),
      m_settings(),
      m_telemetry(),
      m_calibrationState(),
      m_calibrationResult(),
      m_reply(),
      m_hasSettings(false),
      m_hasTelemetry(false),
      m_hasCalibrationState(false),
      m_hasCalibrationResult(false),
      m_hasReply(false),
      m_nowMs(0UL)
{
}

void HardwareControlWeb::begin()
{
    m_calibrationHistoryReady = m_calibrationHistory.begin();
    m_server.on("/api/hardware/hall", HTTP_GET,
                [this]() { handleState(); });
    m_server.on("/api/hardware/hall/refresh", HTTP_POST,
                [this]() { handleRefresh(); });
    m_server.on("/api/hardware/hall/settings", HTTP_POST,
                [this]() { handleSave(); });
    m_server.on("/api/hardware/hall/reset", HTTP_POST,
                [this]() { handleReset(); });
    m_server.on("/api/hardware/hall/telemetry", HTTP_GET,
                [this]() { handleTelemetryState(); });
    m_server.on("/api/hardware/hall/telemetry/start", HTTP_POST,
                [this]() { handleTelemetryStart(); });
    m_server.on("/api/hardware/hall/telemetry/stop", HTTP_POST,
                [this]() { handleTelemetryStop(); });
    m_server.on("/api/hardware/hall/calibration", HTTP_GET,
                [this]() { handleCalibrationState(); });
    m_server.on("/api/hardware/hall/calibration/refresh", HTTP_POST,
                [this]() { handleCalibrationRefresh(); });
    m_server.on("/api/hardware/hall/calibration/arm", HTTP_POST,
                [this]() { handleCalibrationArm(); });
    m_server.on("/api/hardware/hall/calibration/abort", HTTP_POST,
                [this]() { handleCalibrationAbort(); });
    m_server.on("/api/hardware/hall/calibration/apply", HTTP_POST,
                [this]() { handleCalibrationApply(); });
    m_server.on("/api/hardware/hall/calibration/history", HTTP_GET,
                [this]() { handleCalibrationHistory(); });
}

void HardwareControlWeb::update(uint32_t nowMs)
{
    m_nowMs = nowMs;

    HallSettingsState settings;
    while (m_receiver.takeHallSettings(settings))
    {
        m_settings = settings;
        m_hasSettings = settings.valid;
    }

    HallTelemetryState telemetry;
    while (m_receiver.takeHallTelemetry(telemetry))
    {
        m_telemetry = telemetry;
        m_hasTelemetry = telemetry.valid;
    }

    HallCalibrationRemoteStateSnapshot calibrationState;
    while (m_receiver.takeHallCalibrationState(calibrationState))
    {
        m_calibrationState = calibrationState;
        m_hasCalibrationState = calibrationState.valid;
    }

    HallCalibrationRemoteResult calibrationResult;
    while (m_receiver.takeHallCalibrationResult(calibrationResult))
    {
        if (calibrationResult.valid)
        {
            const HallCalibrationProposal proposal =
                HallCalibrationAnalyzer::analyzeSummary(
                    calibrationResult.baselineAdc,
                    calibrationResult.minAdc,
                    calibrationResult.maxAdc,
                    calibrationResult.sampleCount,
                    calibrationResult.durationMs);
            calibrationResult.recommendationValid = proposal.valid;
            calibrationResult.recommendedThreshold = proposal.recommendedThreshold;
            calibrationResult.recommendedHysteresis = proposal.recommendedHysteresis;
            calibrationResult.direction = proposal.direction;
            if (m_calibrationHistoryReady &&
                !m_calibrationHistory.recordMeasurement(calibrationResult, nowMs, nullptr))
            {
                m_calibrationHistoryReady = false;
            }
        }
        m_calibrationResult = calibrationResult;
        m_hasCalibrationResult = calibrationResult.valid;
    }

    HardwareControlReply reply;
    while (m_receiver.takeHardwareControlReply(reply))
    {
        m_reply = reply;
        m_hasReply = true;
        if (m_pendingHistoryMeasurementId != 0UL && m_calibrationHistoryReady)
        {
            const HardwareControlReplyResult historyResult =
                m_pendingHistoryAbort ? HardwareControlReplyResult::Cancelled
                                      : reply.result;
            const HallSettingsState* persisted =
                historyResult == HardwareControlReplyResult::Applied &&
                m_hasSettings && m_settings.valid && m_settings.fromEeprom
                    ? &m_settings : nullptr;
            if (!m_calibrationHistory.finalize(m_pendingHistoryMeasurementId,
                                               historyResult,
                                               persisted))
            {
                m_calibrationHistoryReady = false;
            }
            m_pendingHistoryMeasurementId = 0UL;
            m_pendingHistoryAbort = false;
        }
    }
}

bool HardwareControlWeb::parseUnsigned(const String& source,
                                       uint32_t maximum,
                                       uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0')) return false;

    for (size_t index = 0U; index < source.length(); ++index)
    {
        if (!isDigit(source[index])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[index] - '0');
        if (value > (0xFFFFFFFFUL - digit) / 10UL) return false;
        value = value * 10UL + digit;
    }
    return value <= maximum;
}

const char* HardwareControlWeb::directionText(HallSignalDirectionRemote direction)
{
    return direction == HallSignalDirectionRemote::Falling ? "FALLING" : "RISING";
}

const char* HardwareControlWeb::calibrationStateText(
    HallCalibrationRemoteState state)
{
    switch (state)
    {
        case HallCalibrationRemoteState::WaitingLocalConfirm:
            return "WAITING_LOCAL_CONFIRM";
        case HallCalibrationRemoteState::ArmedWaitingPhysicalStart:
            return "ARMED_WAITING_START";
        case HallCalibrationRemoteState::Running:
            return "RUNNING";
        case HallCalibrationRemoteState::Completed:
            return "COMPLETED";
        case HallCalibrationRemoteState::WaitingApplyConfirm:
            return "WAITING_APPLY_CONFIRM";
        case HallCalibrationRemoteState::Aborted:
            return "ABORTED";
        case HallCalibrationRemoteState::Idle:
        default:
            return "IDLE";
    }
}

const char* HardwareControlWeb::replyText(HardwareControlReplyResult result)
{
    switch (result)
    {
        case HardwareControlReplyResult::Applied: return "APPLIED";
        case HardwareControlReplyResult::Busy: return "BUSY";
        case HardwareControlReplyResult::Invalid: return "INVALID";
        case HardwareControlReplyResult::IdentityMismatch:
            return "IDENTITY_MISMATCH";
        case HardwareControlReplyResult::Cancelled: return "CANCELLED";
        case HardwareControlReplyResult::PersistenceFailed:
            return "PERSISTENCE_FAILED";
        case HardwareControlReplyResult::Unsupported: return "UNSUPPORTED";
        case HardwareControlReplyResult::TimedOut: return "TIMED_OUT";
        case HardwareControlReplyResult::None:
        default: return "NONE";
    }
}

bool HardwareControlWeb::fresh(uint32_t nowMs,
                               uint32_t receivedAtMs,
                               uint32_t limitMs)
{
    return receivedAtMs != 0UL &&
           static_cast<uint32_t>(nowMs - receivedAtMs) <= limitMs;
}

bool HardwareControlWeb::queueAccepted(bool accepted, const char* errorName)
{
    if (!accepted)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      String(F("{\"error\":\"")) + errorName + F("\"}"));
        return false;
    }

    m_hasReply = false;
    m_server.send(202, "application/json; charset=utf-8",
                  "{\"queued\":true,\"pending\":true}");
    return true;
}

void HardwareControlWeb::handleState()
{
    String response;
    response.reserve(420U);
    response += F("{\"available\":");
    response += m_hasSettings ? F("true") : F("false");
    response += F(",\"pending\":");
    response += m_receiver.hallControlPending() ? F("true") : F("false");
    response += F(",\"fresh\":");
    response += (m_hasSettings && fresh(m_nowMs, m_settings.receivedAtMs,
                                        SettingsFreshMs))
                    ? F("true") : F("false");
    response += F(",\"received_at_ms\":");
    response += m_hasSettings ? m_settings.receivedAtMs : 0UL;

    if (m_hasSettings)
    {
        response += F(",\"threshold\":"); response += m_settings.threshold;
        response += F(",\"hysteresis\":"); response += m_settings.hysteresis;
        response += F(",\"release_debounce_ms\":");
        response += m_settings.releaseDebounceMs;
        response += F(",\"direction\":\"");
        response += directionText(m_settings.direction);
        response += F("\",\"source\":\"");
        response += m_settings.fromEeprom ? F("EEPROM") : F("FACTORY");
        response += '"';
    }

    response += F(",\"last_reply\":\"");
    response += m_hasReply ? replyText(m_reply.result) : "NONE";
    response += F("\",\"last_reply_attempts\":");
    response += m_hasReply ? m_reply.sendAttempts : 0U;
    response += F("}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void HardwareControlWeb::handleRefresh()
{
    queueAccepted(m_receiver.requestHallSettings(), "hall_control_busy");
}

void HardwareControlWeb::handleSave()
{
    if (!m_server.hasArg("threshold") ||
        !m_server.hasArg("hysteresis") ||
        !m_server.hasArg("release_debounce_ms") ||
        !m_server.hasArg("direction"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"hall_settings_fields_required\"}");
        return;
    }

    uint32_t threshold = 0UL;
    uint32_t hysteresis = 0UL;
    uint32_t releaseDebounceMs = 0UL;
    if (!parseUnsigned(m_server.arg("threshold"), 1023UL, threshold) ||
        threshold == 0UL ||
        !parseUnsigned(m_server.arg("hysteresis"), 512UL, hysteresis) ||
        hysteresis == 0UL || hysteresis >= threshold ||
        !parseUnsigned(m_server.arg("release_debounce_ms"), 1000UL,
                       releaseDebounceMs) ||
        releaseDebounceMs == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_hall_settings\"}");
        return;
    }

    HallSignalDirectionRemote direction;
    const String directionArg = m_server.arg("direction");
    if (directionArg == "RISING")
        direction = HallSignalDirectionRemote::Rising;
    else if (directionArg == "FALLING")
        direction = HallSignalDirectionRemote::Falling;
    else
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_hall_direction\"}");
        return;
    }

    queueAccepted(
        m_receiver.setHallSettings(static_cast<uint16_t>(threshold),
                                   static_cast<uint16_t>(hysteresis),
                                   static_cast<uint16_t>(releaseDebounceMs),
                                   direction),
        "hall_control_busy");
}

void HardwareControlWeb::handleReset()
{
    queueAccepted(m_receiver.resetHallSettings(), "hall_control_busy");
}

void HardwareControlWeb::handleTelemetryState()
{
    String response;
    response.reserve(520U);
    response += F("{\"available\":");
    response += m_hasTelemetry ? F("true") : F("false");
    response += F(",\"pending\":");
    response += m_receiver.hallControlPending() ? F("true") : F("false");
    response += F(",\"fresh\":");
    response += (m_hasTelemetry && fresh(m_nowMs, m_telemetry.receivedAtMs,
                                         TelemetryFreshMs))
                    ? F("true") : F("false");
    response += F(",\"received_at_ms\":");
    response += m_hasTelemetry ? m_telemetry.receivedAtMs : 0UL;

    if (m_hasTelemetry)
    {
        response += F(",\"raw\":"); response += m_telemetry.rawAdc;
        response += F(",\"min\":"); response += m_telemetry.windowMin;
        response += F(",\"max\":"); response += m_telemetry.windowMax;
        response += F(",\"threshold\":"); response += m_telemetry.threshold;
        response += F(",\"hysteresis\":"); response += m_telemetry.hysteresis;
        response += F(",\"release_boundary\":");
        response += m_telemetry.releaseBoundary;
        response += F(",\"release_debounce_ms\":");
        response += m_telemetry.releaseDebounceMs;
        response += F(",\"direction\":\"");
        response += directionText(m_telemetry.direction);
        response += F("\",\"magnet_detected\":");
        response += m_telemetry.magnetDetected ? F("true") : F("false");
        response += F(",\"rearm\":\""); response += m_telemetry.rearmState;
        response += F("\",\"samples\":"); response += m_telemetry.sampleCount;
        response += F(",\"captured_ms\":"); response += m_telemetry.capturedAtMs;
    }

    response += F("}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void HardwareControlWeb::handleTelemetryStart()
{
    queueAccepted(m_receiver.setHallTelemetryEnabled(true),
                  "hall_control_busy");
}

void HardwareControlWeb::handleTelemetryStop()
{
    queueAccepted(m_receiver.setHallTelemetryEnabled(false),
                  "hall_control_busy");
}

void HardwareControlWeb::handleCalibrationState()
{
    String response;
    response.reserve(780U);
    response += F("{\"available\":");
    response += m_hasCalibrationState ? F("true") : F("false");
    response += F(",\"pending\":");
    response += m_receiver.hallControlPending() ? F("true") : F("false");
    response += F(",\"fresh\":");
    response += (m_hasCalibrationState &&
                 fresh(m_nowMs, m_calibrationState.receivedAtMs,
                       CalibrationFreshMs))
                    ? F("true") : F("false");
    response += F(",\"received_at_ms\":");
    response += m_hasCalibrationState ? m_calibrationState.receivedAtMs : 0UL;

    if (m_hasCalibrationState)
    {
        response += F(",\"state\":\"");
        response += calibrationStateText(m_calibrationState.state);
        response += F("\",\"baseline_ready\":");
        response += m_calibrationState.baselineReady ? F("true") : F("false");
        response += F(",\"motor_permit\":");
        response += m_calibrationState.motorPermit ? F("true") : F("false");
    }

    response += F(",\"result_available\":");
    response += m_hasCalibrationResult ? F("true") : F("false");
    if (m_hasCalibrationResult)
    {
        response += F(",\"result_received_at_ms\":");
        response += m_calibrationResult.receivedAtMs;
        response += F(",\"measurement_id\":");
        response += m_calibrationResult.measurementId;
        response += F(",\"recommendation_valid\":");
        response += m_calibrationResult.recommendationValid
                        ? F("true") : F("false");
        response += F(",\"baseline\":");
        response += m_calibrationResult.baselineAdc;
        response += F(",\"min\":");
        response += m_calibrationResult.minAdc;
        response += F(",\"max\":");
        response += m_calibrationResult.maxAdc;
        response += F(",\"recommended_threshold\":");
        response += m_calibrationResult.recommendedThreshold;
        response += F(",\"recommended_hysteresis\":");
        response += m_calibrationResult.recommendedHysteresis;
        response += F(",\"recommended_direction\":\"");
        response += directionText(m_calibrationResult.direction);
        response += F("\",\"samples\":");
        response += m_calibrationResult.sampleCount;
        response += F(",\"duration_ms\":");
        response += m_calibrationResult.durationMs;
    }

    response += F(",\"last_reply\":\"");
    response += m_hasReply ? replyText(m_reply.result) : "NONE";
    response += F("\",\"last_reply_attempts\":");
    response += m_hasReply ? m_reply.sendAttempts : 0U;
    response += F(",\"history_ready\":");
    response += m_calibrationHistoryReady ? F("true") : F("false");
    response += F("}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void HardwareControlWeb::handleCalibrationRefresh()
{
    queueAccepted(m_receiver.requestHallCalibration(), "hall_control_busy");
}

void HardwareControlWeb::handleCalibrationArm()
{
    m_hasCalibrationResult = false;
    m_pendingHistoryMeasurementId = 0UL;
    m_pendingHistoryAbort = false;
    queueAccepted(m_receiver.armHallCalibration(), "hall_control_busy");
}

void HardwareControlWeb::handleCalibrationAbort()
{
    const bool accepted = m_receiver.abortHallCalibration();
    if (accepted && m_pendingHistoryMeasurementId != 0UL)
        m_pendingHistoryAbort = true;
    queueAccepted(accepted, "hall_control_busy");
}

void HardwareControlWeb::handleCalibrationApply()
{
    if (!m_server.hasArg("release_debounce_ms"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"release_debounce_required\"}");
        return;
    }

    uint32_t releaseDebounceMs = 0UL;
    if (!parseUnsigned(m_server.arg("release_debounce_ms"), 1000UL,
                       releaseDebounceMs) || releaseDebounceMs == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_release_debounce\"}");
        return;
    }

    if (!m_hasCalibrationResult || !m_calibrationResult.valid ||
        !m_calibrationResult.recommendationValid ||
        m_calibrationResult.measurementId == 0UL ||
        !m_hasCalibrationState ||
        m_calibrationState.state != HallCalibrationRemoteState::Completed)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"calibration_result_not_applicable\"}");
        return;
    }

    const uint32_t measurementId = m_calibrationResult.measurementId;
    const bool accepted = m_receiver.proposeHallCalibration(
        measurementId,
        m_calibrationResult.recommendedThreshold,
        m_calibrationResult.recommendedHysteresis,
        static_cast<uint16_t>(releaseDebounceMs),
        m_calibrationResult.direction);
    if (accepted)
    {
        m_pendingHistoryMeasurementId = measurementId;
        m_pendingHistoryAbort = false;
    }
    queueAccepted(accepted, "hall_control_busy");
}

void HardwareControlWeb::handleCalibrationHistory()
{
    if (!m_calibrationHistoryReady)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"hall_calibration_history_unavailable\"}");
        return;
    }

    HallCalibrationHistoryEntry entries[HallCalibrationHistoryStore::MaxEntries];
    uint8_t count = 0U;
    if (!m_calibrationHistory.load(entries, count))
    {
        m_calibrationHistoryReady = false;
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"hall_calibration_history_read_failed\"}");
        return;
    }

    String response;
    response.reserve(640U + static_cast<size_t>(count) * 420U);
    response += F("{\"available\":true,\"limit\":10,\"count\":");
    response += count;
    response += F(",\"items\":[");
    for (int index = static_cast<int>(count) - 1; index >= 0; --index)
    {
        const HallCalibrationHistoryEntry& entry = entries[index];
        if (index != static_cast<int>(count) - 1) response += ',';
        response += F("{\"measurement_id\":"); response += entry.measurementId;
        response += F(",\"recorded_at_ms\":"); response += entry.recordedAtMs;
        response += F(",\"rtc_valid\":"); response += entry.rtcValid ? F("true") : F("false");
        response += F(",\"baseline\":"); response += entry.baselineAdc;
        response += F(",\"min\":"); response += entry.minAdc;
        response += F(",\"max\":"); response += entry.maxAdc;
        response += F(",\"span\":"); response += static_cast<uint16_t>(entry.maxAdc - entry.minAdc);
        response += F(",\"samples\":"); response += entry.sampleCount;
        response += F(",\"duration_ms\":"); response += entry.durationMs;
        response += F(",\"recommendation_valid\":");
        response += entry.recommendationValid ? F("true") : F("false");
        response += F(",\"recommended_threshold\":"); response += entry.recommendedThreshold;
        response += F(",\"recommended_hysteresis\":"); response += entry.recommendedHysteresis;
        response += F(",\"recommended_direction\":\"");
        response += directionText(entry.recommendedDirection);
        response += F("\",\"apply_result\":\""); response += replyText(entry.applyResult);
        response += F("\",\"persisted_valid\":");
        response += entry.persistedProfileValid ? F("true") : F("false");
        if (entry.persistedProfileValid)
        {
            response += F(",\"persisted_threshold\":"); response += entry.persistedThreshold;
            response += F(",\"persisted_hysteresis\":"); response += entry.persistedHysteresis;
            response += F(",\"persisted_release_debounce_ms\":");
            response += entry.persistedReleaseDebounceMs;
            response += F(",\"persisted_direction\":\"");
            response += directionText(entry.persistedDirection);
            response += '"';
        }
        response += '}';
    }
    response += F("]}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

} // namespace CM