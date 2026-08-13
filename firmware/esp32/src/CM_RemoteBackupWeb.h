#ifndef CM_REMOTE_BACKUP_WEB_H
#define CM_REMOTE_BACKUP_WEB_H

#include <FS.h>
#include <WebServer.h>

#include "CM_RemoteBackupSettings.h"
#include "CM_RemoteBackupTransfer.h"

namespace CM
{
class RemoteBackupWeb
{
public:
    RemoteBackupWeb(WebServer& server,
                    fs::FS& storage,
                    RemoteBackupSettingsStore& settingsStore);

    void begin();
    void update(uint32_t nowMs);
    void setActivityProbe(BackupActivityGuard::RuntimeProbe activityProbe);

private:
    void handleGetConfiguration();
    void handleSetConfiguration();
    void handleTestConnection();
    void handleStartUpload();
    void handleUploadStatus();

    WebServer& m_server;
    fs::FS& m_storage;
    RemoteBackupSettingsStore& m_settingsStore;
    RemoteBackupTransfer m_transfer;
};
}

#endif // CM_REMOTE_BACKUP_WEB_H
