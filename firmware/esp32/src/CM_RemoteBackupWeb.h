#ifndef CM_REMOTE_BACKUP_WEB_H
#define CM_REMOTE_BACKUP_WEB_H

#include <FS.h>
#include <WebServer.h>

#include "CM_RemoteBackupSettings.h"
#include "CM_RemoteBackupTransfer.h"
#include "CM_RtcClock.h"

namespace CM
{
class RemoteBackupWeb
{
public:
    RemoteBackupWeb(WebServer& server,
                    fs::FS& storage,
                    RemoteBackupSettingsStore& settingsStore,
                    RtcClock& rtcClock);

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
    bool startFullBatch(bool scheduled,
                        uint16_t& statusCode,
                        const char*& error);
    void updateSchedule(uint32_t nowMs);
    void completeScheduledBatch();
    static uint32_t dateKey(const RtcDateTime& value);
    bool startNextBatchFile();
    bool startTrackedBatchFile(const String& logicalName,
                               const String& localPath,
                               const String& remoteName);
    bool createRemoteBatchManifest();
    bool createCompletionMarker();
    bool beginBatchManifest();
    bool appendBatchManifestName(const String& remoteName);
    bool finalizeBatchManifest();
    bool startRetention();
    bool selectOldestManagedBatch(uint32_t& batchId,
                                  uint16_t& manifestCount) const;
    bool selectOldestIncompleteBatch(uint32_t& batchId) const;
    bool startIncompleteCleanup(uint32_t batchId);
    bool startNextIncompleteDelete();
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
        Manifest,
        Marker,
        Retention,
        Complete,
        Failed
    };

    WebServer& m_server;
    fs::FS& m_storage;
    RemoteBackupSettingsStore& m_settingsStore;
    RtcClock& m_rtcClock;
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
    uint16_t m_incompleteBatchesDeleted = 0U;
    bool m_retentionMarkerDeleted = false;
    bool m_retentionSucceeded = true;
    bool m_retentionOnly = false;
    bool m_retentionIncomplete = false;
    bool m_retentionDeletingPart = false;
    String m_retentionPendingName;
    String m_retentionError;
    uint32_t m_lastScheduleCheckMs = 0UL;
    uint32_t m_scheduleAttemptDate = 0UL;
    uint32_t m_scheduledBatchDate = 0UL;
    bool m_batchScheduled = false;
    String m_scheduleState = "DISABLED";
};
}

#endif // CM_REMOTE_BACKUP_WEB_H
