#ifndef CM_REMOTE_BACKUP_WEB_H
#define CM_REMOTE_BACKUP_WEB_H

#include <FS.h>
#include <WebServer.h>

#include "CM_RemoteBackupSettings.h"

namespace CM
{
class RemoteBackupWeb
{
public:
    RemoteBackupWeb(WebServer& server,
                    fs::FS& storage,
                    RemoteBackupSettingsStore& settingsStore);

    void begin();

private:
    void handleGetConfiguration();
    void handleSetConfiguration();
    void handleTestConnection();

    WebServer& m_server;
    fs::FS& m_storage;
    RemoteBackupSettingsStore& m_settingsStore;
};
}

#endif // CM_REMOTE_BACKUP_WEB_H
