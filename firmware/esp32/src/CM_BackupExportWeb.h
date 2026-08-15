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

    // Compatibility registration path. Production wiring may register these
    // handlers through an external activity guard instead.
    void begin();
    bool ready() const;

    void handleManifest();
    void handleFile();
    void handleSessions();
    void handleSessionFile();

    // Resolves only the fixed backup whitelist. The returned path is never
    // derived from a request path.
    static bool resolveExportFile(const String& name,
                                  String& path,
                                  String& downloadName);
    static bool snapshotStable(fs::FS& storage, String& reason);
    static size_t exportFileCount();
    static bool resolveExportFileAt(size_t index,
                                    String& logicalName,
                                    String& path,
                                    String& downloadName);
    static bool nextSessionId(fs::FS& storage,
                              uint32_t afterSessionId,
                              uint32_t& sessionId,
                              bool& found);
    static bool resolveSessionFile(fs::FS& storage,
                                   uint32_t sessionId,
                                   uint8_t kindIndex,
                                   String& logicalName,
                                   String& path,
                                   String& downloadName,
                                   bool& exists);
    // Maps only canonical names produced by a managed full backup to fixed
    // restore destinations. No path supplied by a manifest is accepted.
    static bool resolveRestoreTarget(uint32_t batchId,
                                     const String& remoteName,
                                     String& logicalName,
                                     String& targetPath);

private:
    WebServer& m_server;
    fs::FS& m_storage;
};
}

#endif // CM_BACKUP_EXPORT_WEB_H
