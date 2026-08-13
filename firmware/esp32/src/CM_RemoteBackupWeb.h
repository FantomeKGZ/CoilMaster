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
    void handleBatchStatus();
    bool allocateBatchId(uint32_t& batchId);
    bool startNextBatchFile();
    bool createCompletionMarker();
    void failBatch(const char* reason);

    enum class BatchStage : uint8_t
    {
        Idle,
        MainFiles,
        SessionFiles,
        Marker,
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
};
}

#endif // CM_REMOTE_BACKUP_WEB_H
