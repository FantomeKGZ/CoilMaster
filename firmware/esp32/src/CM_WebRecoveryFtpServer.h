#ifndef CM_WEB_RECOVERY_FTP_SERVER_H
#define CM_WEB_RECOVERY_FTP_SERVER_H

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "CM_BackupActivityGuard.h"

namespace CM
{
class WebRecoveryFtpServer
{
public:
    WebRecoveryFtpServer(WebServer& webServer, fs::FS& storage);

    // Registers the operator API and optionally starts the recovery server.
    // automaticRecovery must be captured before /web is created.
    void begin(bool automaticRecovery);
    void update(uint32_t nowMs);
    void setActivityProbe(BackupActivityGuard::RuntimeProbe probe);

    bool running() const;
    bool automaticRecovery() const;
    bool clientConnected();
    bool transferActive() const;
    const String& lastResult() const;

private:
    enum class TransferState : uint8_t
    {
        None = 0U,
        AwaitStore,
        Storing,
        AwaitList,
        Listing
    };

    static constexpr uint16_t ControlPort = 21U;
    static constexpr uint16_t PassivePort = 50009U;
    static constexpr size_t MaxCommandLength = 192U;
    static constexpr size_t MaxPathLength = 176U;
    static constexpr uint32_t ClientTimeoutMs = 120000UL;
    static constexpr uint32_t DataTimeoutMs = 15000UL;

    bool start(bool automaticRecovery);
    void stop(const char* result);
    void handleStatus();
    void handleStart();
    void handleStop();
    void acceptControlClient(uint32_t nowMs);
    void readControl(uint32_t nowMs);
    void processCommand(const String& line, uint32_t nowMs);
    void updateTransfer(uint32_t nowMs);
    void abortTransfer(const char* result, bool sendReply);
    void finishStore();
    void finishList();
    void resetSession();
    void closePassive();

    bool activitySafe() const;
    bool loggedInOrReply();
    bool openPassiveOrReply();
    bool resolvePath(const String& argument, String& virtualPath,
                     String& storagePath) const;
    bool sameAccessPointSubnet(const IPAddress& address) const;
    bool sendListEntry();
    void reply(uint16_t code, const String& text);
    void enterPassive(bool extended);

    WebServer& m_webServer;
    fs::FS& m_storage;
    WiFiServer m_controlServer;
    WiFiServer m_dataServer;
    WiFiClient m_controlClient;
    WiFiClient m_dataClient;
    File m_transferFile;
    File m_listDirectory;
    BackupActivityGuard::RuntimeProbe m_activityProbe;
    TransferState m_transferState;
    String m_commandBuffer;
    String m_currentDirectory;
    String m_transferPath;
    String m_transferTemporaryPath;
    String m_renameFrom;
    String m_lastResult;
    uint32_t m_lastClientActivityMs;
    uint32_t m_dataDeadlineMs;
    bool m_running;
    bool m_automaticRecovery;
    bool m_userAccepted;
    bool m_authenticated;
    bool m_passiveOpen;
    bool m_listNamesOnly;
};
}

#endif // CM_WEB_RECOVERY_FTP_SERVER_H
