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
    void handleStartBatch();
    void handleStartRetention();
    void handleBatchStatus();
    bool allocateBatchId(uint32_t& batchId);
    bool startNextBatchFile();
    bool createCompletionMarker();
    bool beginBatchManifest();
    bool appendBatchManifestName(const String& remoteName);
    bool finalizeBatchManifest();
    bool startRetention();
    bool selectOldestManagedBatch(uint32_t& batchId,
                                  uint16_t& manifestCount) const;
    bool startRetentionBatch(uint32_t batchId);
    bool startNextRetentionFile();
    void failRetention(const char* reason);
    String batchManifestPath(uint32_t batchId, bool temporary) const;
    void failBatch(const char* reason);

    enum class BatchStage : uint8_t
    {
        Idle,
        MainFiles,
        SessionFiles,
        Marker,
        Retention,
        Complete,
        Failed
    };

    WebServer& m_server;
    fs::FS& m_storage;
    RemoteBackupSettingsStore& m_settingsStore;
    RemoteBackupTransfer m_transfer;
    RemoteBackupSettings m_batchSettings;
    BatchStage m_batchStage = BatchStage::Idle;
    size_t m_batchMainIndex = 0U;
    uint32_t m_batchAfterSessionId = 0UL;
    uint32_t m_batchSessionId = 0UL;
    uint32_t m_batchId = 0UL;
    uint32_t m_batchFilesCompleted = 0UL;
    uint8_t m_batchKindIndex = 0U;
    String m_batchError;
    File m_retentionManifest;
    uint32_t m_retentionBatchId = 0UL;
    uint32_t m_retentionFilesDeleted = 0UL;
    bool m_retentionMarkerDeleted = false;
    bool m_retentionSucceeded = true;
    bool m_retentionOnly = false;
    String m_retentionError;
};
}

#endif // CM_REMOTE_BACKUP_WEB_H
