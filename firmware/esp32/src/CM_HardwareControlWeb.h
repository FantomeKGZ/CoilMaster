#ifndef CM_HARDWARE_CONTROL_WEB_H
#define CM_HARDWARE_CONTROL_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_UartEventReceiver.h"

namespace CM
{

class HardwareControlWeb
{
public:
    HardwareControlWeb(WebServer& server, UartEventReceiver& receiver);

    void begin();
    void update(uint32_t nowMs);

private:
    static constexpr uint32_t SettingsFreshMs = 15000UL;
    static constexpr uint32_t TelemetryFreshMs = 1500UL;
    static constexpr uint32_t CalibrationFreshMs = 5000UL;

    void handleState();
    void handleRefresh();
    void handleSave();
    void handleReset();
    void handleTelemetryState();
    void handleTelemetryStart();
    void handleTelemetryStop();
    void handleCalibrationState();
    void handleCalibrationRefresh();
    void handleCalibrationArm();
    void handleCalibrationAbort();

    bool queueAccepted(bool accepted, const char* errorName);
    static bool parseUnsigned(const String& source,
                              uint32_t maximum,
                              uint32_t& value);
    static const char* directionText(HallSignalDirectionRemote direction);
    static const char* calibrationStateText(HallCalibrationRemoteState state);
    static const char* replyText(HardwareControlReplyResult result);
    static bool fresh(uint32_t nowMs, uint32_t receivedAtMs, uint32_t limitMs);

    WebServer& m_server;
    UartEventReceiver& m_receiver;
    HallSettingsState m_settings;
    HallTelemetryState m_telemetry;
    HallCalibrationRemoteStateSnapshot m_calibrationState;
    HallCalibrationRemoteResult m_calibrationResult;
    HardwareControlReply m_reply;
    bool m_hasSettings;
    bool m_hasTelemetry;
    bool m_hasCalibrationState;
    bool m_hasCalibrationResult;
    bool m_hasReply;
    uint32_t m_nowMs;
};

} // namespace CM

#endif // CM_HARDWARE_CONTROL_WEB_H
