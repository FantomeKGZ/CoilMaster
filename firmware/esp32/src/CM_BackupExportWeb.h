#ifndef CM_BACKUP_EXPORT_WEB_H
#define CM_BACKUP_EXPORT_WEB_H

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

namespace CM
{
class BackupExportWeb
{
public:
    BackupExportWeb(WebServer& server, fs::FS& storage);

    void begin();
    bool ready() const;

private:
    void handleManifest();
    void handleFile();
    void handleSessions();
    void handleSessionFile();

    WebServer& m_server;
    fs::FS& m_storage;
};
}

#endif // CM_BACKUP_EXPORT_WEB_H
