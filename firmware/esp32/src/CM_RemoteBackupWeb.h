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
    void handleStartInspection();
    void handleInspectionStatus();
    void handleStartStaging();
    void handleStagingStatus();
    void handleDiscardStaging();
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
    bool appendBatchManifestEntry(const String& remoteName,
                                  uint32_t expectedBytes);
    bool finalizeBatchManifest();
    bool validateInspectionManifest(uint32_t& dataFiles);
    bool validateInspectionMarker() const;
    bool startNextInspectionFile();
    void failInspection(const char* reason);
    bool startNextStagingFile();
    bool clearStagingDirectory();
    void failStaging(const char* reason);
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

    enum class InspectionStage : uint8_t
    {
        Idle,
        Manifest,
        Marker,
        Files,
        Complete,
        Failed
    };

    enum class StagingStage : uint8_t
    {
        Idle,
        Files,
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
    InspectionStage m_inspectionStage = InspectionStage::Idle;
    uint32_t m_inspectionBatchId = 0UL;
    uint32_t m_inspectionDataFiles = 0UL;
    uint32_t m_inspectionTotalBytes = 0UL;
    uint32_t m_inspectionFilesVerified = 0UL;
    uint32_t m_inspectionExpectedBytes = 0UL;
    bool m_inspectionHasSizes = false;
    bool m_inspectionExpectedSizeValid = false;
    File m_inspectionManifest;
    String m_inspectionError;
    StagingStage m_stagingStage = StagingStage::Idle;
    File m_stagingManifest;
    uint32_t m_stagingFilesCompleted = 0UL;
    uint32_t m_stagingExpectedBytes = 0UL;
    uint32_t m_stagingBytesCompleted = 0UL;
    String m_stagingError;
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
