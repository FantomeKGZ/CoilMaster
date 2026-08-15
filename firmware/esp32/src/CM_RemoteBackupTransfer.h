#ifndef CM_REMOTE_BACKUP_TRANSFER_H
#define CM_REMOTE_BACKUP_TRANSFER_H

#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>

#include "CM_BackupActivityGuard.h"
#include "CM_RemoteBackupSettings.h"

namespace CM
{
class RemoteBackupTransfer
{
public:
    explicit RemoteBackupTransfer(fs::FS& storage);
    void setActivityProbe(BackupActivityGuard::RuntimeProbe activityProbe);

    bool start(const RemoteBackupSettings& settings,
               const String& logicalName,
               const String& localPath,
               const String& remoteName);
    bool startDelete(const RemoteBackupSettings& settings,
                     const String& logicalName,
                     const String& remoteName);
    bool startDownload(const RemoteBackupSettings& settings,
                       const String& logicalName,
                       const String& remoteName,
                       const String& localPath,
                       uint32_t maximumBytes);
    void finishDeleteSession();
    void update(uint32_t nowMs);
    bool active() const;
    bool succeeded() const;
    const char* stateName() const;
    const String& error() const;
    const String& logicalName() const;
    const String& remoteName() const;
    uint32_t bytesTotal() const;
    uint32_t bytesSent() const;

private:
    enum class Phase : uint8_t
    {
        Idle,
        Greeting,
        User,
        Password,
        ChangeDirectory,
        BinaryMode,
        DeleteTemp,
        Passive,
        StoreReady,
        Sending,
        StoreComplete,
        Size,
        DownloadSize,
        RetrieveReady,
        Receiving,
        RetrieveComplete,
        DeleteFinal,
        RenameFrom,
        RenameTo,
        DeleteOnly,
        Complete,
        Failed
    };

    enum class Operation : uint8_t
    {
        Upload,
        Delete,
        Download
    };

    bool readReply(uint16_t& code, String& line);
    bool sendCommand(const String& command, Phase next, uint32_t nowMs);
    bool openPassiveData(const String& reply, uint32_t nowMs);
    void fail(const char* reason);
    void closeTransfer();
    void resetDeadline(uint32_t nowMs);

    fs::FS& m_storage;
    BackupActivityGuard::RuntimeProbe m_activityProbe;
    WiFiClient m_control;
    WiFiClient m_data;
    File m_file;
    RemoteBackupSettings m_settings;
    String m_logicalName;
    String m_remoteName;
    String m_tempName;
    String m_localPath;
    String m_localTempPath;
    String m_replyLine;
    String m_error;
    Operation m_operation;
    Phase m_phase;
    uint32_t m_deadlineMs;
    uint32_t m_totalBytes;
    uint32_t m_sentBytes;
    uint32_t m_downloadLimitBytes;
};
}

#endif // CM_REMOTE_BACKUP_TRANSFER_H
