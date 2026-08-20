#ifndef CM_HARDWARE_CONTROL_CLIENT_H
#define CM_HARDWARE_CONTROL_CLIENT_H

#include <Arduino.h>

namespace CM
{

enum class HallSignalDirectionRemote : uint8_t
{
    Rising = 0U,
    Falling
};

struct HallSettingsState
{
    uint16_t threshold;
    uint16_t hysteresis;
    uint16_t releaseDebounceMs;
    HallSignalDirectionRemote direction;
    bool fromEeprom;
    bool valid;
    uint32_t receivedAtMs;

    HallSettingsState();
};

struct HallTelemetryState
{
    uint16_t rawAdc;
    uint16_t windowMin;
    uint16_t windowMax;
    uint16_t threshold;
    uint16_t hysteresis;
    uint16_t releaseBoundary;
    uint16_t releaseDebounceMs;
    HallSignalDirectionRemote direction;
    bool magnetDetected;
    char rearmState[24];
    uint16_t sampleCount;
    uint32_t capturedAtMs;
    uint32_t receivedAtMs;
    bool valid;

    HallTelemetryState();
};

enum class HallCalibrationRemoteState : uint8_t
{
    Idle = 0U,
    ArmedWaitingPhysicalStart,
    Running,
    Completed,
    Aborted
};

struct HallCalibrationRemoteStateSnapshot
{
    HallCalibrationRemoteState state;
    bool baselineReady;
    bool motorPermit;
    bool valid;
    uint32_t receivedAtMs;

    HallCalibrationRemoteStateSnapshot();
};

struct HallCalibrationRemoteResult
{
    bool recommendationValid;
    uint16_t baselineAdc;
    uint16_t minAdc;
    uint16_t maxAdc;
    uint16_t recommendedThreshold;
    uint16_t recommendedHysteresis;
    HallSignalDirectionRemote direction;
    uint16_t sampleCount;
    uint32_t durationMs;
    bool valid;
    uint32_t receivedAtMs;

    HallCalibrationRemoteResult();
};

enum class HardwareControlReplyResult : uint8_t
{
    None = 0U,
    Applied,
    Busy,
    Invalid,
    PersistenceFailed,
    Unsupported,
    TimedOut
};

struct HardwareControlReply
{
    HardwareControlReplyResult result;
    uint8_t sendAttempts;

    HardwareControlReply();
};

class HardwareControlClient
{
public:
    explicit HardwareControlClient(HardwareSerial& serial);

    void update(uint32_t nowMs);
    bool processLine(char* line, uint32_t nowMs);

    bool requestSettings();
    bool setSettings(uint16_t threshold,
                     uint16_t hysteresis,
                     uint16_t releaseDebounceMs,
                     HallSignalDirectionRemote direction);
    bool resetSettings();
    bool setTelemetryEnabled(bool enabled);
    bool armHallCalibration();
    bool abortHallCalibration();
    bool requestHallCalibration();

    bool requestPending() const;
    bool takeSettings(HallSettingsState& state);
    bool takeTelemetry(HallTelemetryState& state);
    bool takeHallCalibrationState(HallCalibrationRemoteStateSnapshot& state);
    bool takeHallCalibrationResult(HallCalibrationRemoteResult& result);
    bool takeReply(HardwareControlReply& reply);

private:
    enum class RequestType : uint8_t
    {
        None = 0U,
        GetSettings,
        SetSettings,
        ResetSettings,
        StartTelemetry,
        StopTelemetry,
        ArmCalibration,
        AbortCalibration,
        GetCalibration
    };

    static constexpr uint32_t RetryIntervalMs = 1000UL;
    static constexpr uint8_t MaxSendAttempts = 3U;
    static constexpr size_t MaxRequestPayloadLength = 96U;

    bool queueRequest(RequestType type, const char* payload);
    bool sendPending(uint32_t nowMs);
    bool processSettingsState(char* line, uint32_t nowMs);
    bool processSettingsResult(char* line);
    bool processTelemetryState(char* line, uint32_t nowMs);
    bool processCalibrationState(char* line, uint32_t nowMs);
    bool processCalibrationResult(char* line, uint32_t nowMs);
    void finishRequest(HardwareControlReplyResult result);

    static bool validateAndStripCrc(char* line);
    static bool parseDecimal16(const char* text, uint16_t& value);
    static bool parseDecimal32(const char* text, uint32_t& value);
    static bool parseHex16(const char* text, uint16_t& value);
    static HardwareControlReplyResult parseResultName(const char* text);

    HardwareSerial& m_serial;
    RequestType m_requestType;
    char m_requestPayload[MaxRequestPayloadLength];
    bool m_waitingReply;
    uint32_t m_lastSendMs;
    uint8_t m_sendAttempts;

    HallSettingsState m_settings;
    bool m_hasSettings;
    HallTelemetryState m_telemetry;
    bool m_hasTelemetry;
    HallCalibrationRemoteStateSnapshot m_calibrationState;
    bool m_hasCalibrationState;
    HallCalibrationRemoteResult m_calibrationResult;
    bool m_hasCalibrationResult;
    HardwareControlReply m_reply;
    bool m_hasReply;
};

} // namespace CM

#endif // CM_HARDWARE_CONTROL_CLIENT_H
