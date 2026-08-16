Warning: truncated output (original token count: 40869)
Total output lines: 4097

#include "CM_RemoteBackupWeb.h"

#include <WiFi.h>

#include "CM_BackupActivityGuard.h"
#include "CM_BackupExportWeb.h"

namespace CM
{
namespace
{
constexpr const char* BatchSequencePath =
    "/data/settings/remote-backup-sequence.txt";
constexpr const char* BatchSequenceTempPath =
    "/data/settings/remote-backup-sequence.tmp";
constexpr const char* BatchSequenceBackupPath =
    "/data/settings/remote-backup-sequence.bak";
constexpr const char* BatchMarkerPath =
    "/data/settings/remote-backup-complete.txt";
constexpr const char* RemoteBatchManifestPath =
    "/data/settings/remote-backup-manifest.txt";
constexpr const char* BatchManifestDirectory =
    "/data/settings/remote-backup-batches";
constexpr const char* InspectionDirectory =
    "/data/settings/remote-backup-inspection";
constexpr const char* InspectionManifestPath =
    "/data/settings/remote-backup-inspection/MANIFEST.txt";
constexpr const char* InspectionMarkerPath =
    "/data/settings/remote-backup-inspection/COMPLETE.txt";
constexpr const char* StagingDirectory =
    "/data/settings/remote-restore-staging";
constexpr const char* StagingMarkerPath =
    "/data/settings/remote-restore-staging/STAGED.txt";
constexpr const char* RestorePlanPath =
    "/data/settings/remote-restore-staging/RESTORE_PLAN.tsv";
constexpr const char* RestorePlanTempPath =
    "/data/settings/remote-restore-staging/RESTORE_PLAN.tsv.part";
constexpr const char* RollbackDirectory =
    "/data/settings/remote-restore-rollback";
constexpr const char* RollbackManifestPath =
    "/data/settings/remote-restore-rollback/FILES.tsv";
constexpr const char* RollbackManifestTempPath =
    "/data/settings/remote-restore-rollback/FILES.tsv.part";
constexpr const char* RollbackMarkerPath =
    "/data/settings/remote-restore-rollback/ROLLBACK.txt";
constexpr const char* ApplyPreflightPath =
    "/data/settings/remote-restore-rollback/APPLY_PREFLIGHT.tsv";
constexpr const char* ApplyPreflightTempPath =
    "/data/settings/remote-restore-rollback/APPLY_PREFLIGHT.tsv.part";
constexpr const char* ApplyReadyMarkerPath =
    "/data/settings/remote-restore-rollback/APPLY_READY.txt";
constexpr const char* ApplyJournalPath =
    "/data/settings/remote-restore-rollback/APPLY_JOURNAL.tsv";
constexpr const char* ApplyResultMarkerPath =
    "/data/settings/remote-restore-rollback/APPLY_RESULT.txt";

bool parseCanonicalUnsigned(const String& source,
                            uint32_t minimum,
                            uint32_t maximum,
                            uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U ||
        (source.length() > 1U && source[0] == '0'))
    {
        return false;
    }
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        if (!isDigit(source[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(source[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

bool parseBoolean(const String& source, bool& value)
{
    if (source == "1" || source == "true")
    {
        value = true;
        return true;
    }
    if (source == "0" || source == "false")
    {
        value = false;
        return true;
    }
    return false;
}

bool readInspectionLine(File& file, String& line)
{
    if (!file.available()) return false;
    line = file.readStringUntil('\n');
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    return line.length() <= 512U;
}

bool parseManagedManifestEntry(const String& source,
                               uint32_t batchId,
                               String& remoteName,
                               bool& hasExpectedBytes,
                               uint32_t& expectedBytes)
{
    String line = source;
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    hasExpectedBytes = false;
    expectedBytes = 0UL;
    const int separator = line.indexOf('\t');
    if (separator >= 0)
    {
        if (line.indexOf('\t', separator + 1) >= 0) return false;
        const String sizeText = line.substring(separator + 1);
        if (!parseCanonicalUnsigned(sizeText, 0UL, 0xFFFFFFFFUL,
                                    expectedBytes)) return false;
        remoteName = line.substring(0U, static_cast<unsigned>(separator));
        hasExpectedBytes = true;
    }
    else remoteName = line;

    const String prefix = String(F("cm-b")) + batchId + '-';
    return remoteName.length() > 0U && remoteName.length() <= 180U &&
           remoteName.startsWith(prefix) && remoteName.indexOf('/') < 0 &&
           remoteName.indexOf("..") < 0 && remoteName.indexOf('\r') < 0 &&
           remoteName.indexOf('\n') < 0;
}

bool parseRestorePlanEntry(const String& source,
                           String& remoteName,
                           uint32_t& expectedBytes,
                           String& logicalName,
                           String& targetPath)
{
    String line = source;
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    if (line.length() == 0U || line.length() > 420U) return false;
    const int first = line.indexOf('\t');
    const int second = first >= 0 ? line.indexOf('\t', first + 1) : -1;
    const int third = second >= 0 ? line.indexOf('\t', second + 1) : -1;
    if (first <= 0 || second <= first + 1 || third <= second + 1 ||
        line.indexOf('\t', third + 1) >= 0) return false;
    remoteName = line.substring(0U, static_cast<unsigned>(first));
    const String sizeText = line.substring(static_cast<unsigned>(first + 1),
                                           static_cast<unsigned>(second));
    logicalName = line.substring(static_cast<unsigned>(second + 1),
                                 static_cast<unsigned>(third));
    targetPath = line.substring(static_cast<unsigned>(third + 1));
    return parseCanonicalUnsigned(sizeText, 0UL, 536870912UL,
                                  expectedBytes) &&
           remoteName.length() <= 180U && logicalName.length() <= 80U &&
           targetPath.length() > 0U && targetPath.length() <= 160U &&
           remoteName.indexOf('/') < 0 && remoteName.indexOf("..") < 0 &&
           targetPath.startsWith("/data/") &&
           targetPath.indexOf("..") < 0 && targetPath.indexOf('\t') < 0 &&
           targetPath.indexOf('\r') < 0 && targetPath.indexOf('\n') < 0;
}

bool parseHex32(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() != 8U) return false;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        const char ch = source[i];
        uint8_t digit = 0U;
        if (ch >= '0' && ch <= '9') digit = static_cast<uint8_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            digit = static_cast<uint8_t>(ch - 'a' + 10U);
        else return false;
        value = (value << 4U) | digit;
    }
    return true;
}

bool parseRollbackManifestEntry(const String& source,
                                String& remoteName,
                                String& targetPath,
                                bool& present,
                                uint32_t& sizeBytes,
                                uint32_t& crc32)
{
    String line = source;
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    if (line.length() == 0U || line.length() > 460U) return false;
    int separators[4] = {-1, -1, -1, -1};
    int cursor = -1;
    for (uint8_t i = 0U; i < 4U; ++i)
    {
        cursor = line.indexOf('\t', cursor + 1);
        if (cursor < 0) return false;
        separators[i] = cursor;
    }
    if (line.indexOf('\t', separators[3] + 1) >= 0) return false;
    remoteName = line.substring(0U, static_cast<unsigned>(separators[0]));
    targetPath = line.substring(static_cast<unsigned>(separators[0] + 1),
                                static_cast<unsigned>(separators[1]));
    const String state = line.substring(static_cast<unsigned>(separators[1] + 1),
                                        static_cast<unsigned>(separators[2]));
    const String sizeText = line.substring(static_cast<unsigned>(separators[2] + 1),
                                           static_cast<unsigned>(separators[3]));
    const String crcText = line.substring(static_cast<unsigned>(separators[3] + 1));
    if (!parseCanonicalUnsigned(sizeText, 0UL, 1073741824UL, sizeBytes))
        return false;
    const bool safeNames = remoteName.length() > 0U &&
                           remoteName.length() <= 180U &&
                           remoteName.indexOf('/') < 0 &&
                           remoteName.indexOf("..") < 0 &&
                           targetPath.startsWith("/data/") &&
                           targetPath.length() <= 160U &&
                           targetPath.indexOf("..") < 0 &&
                           targetPath.indexOf('\t') < 0 &&
                           targetPath.indexOf('\r') < 0 &&
                           targetPath.indexOf('\n') < 0;
    if (!safeNames) return false;
    if (state == F("MISSING"))
    {
        present = false;
        crc32 = 0UL;
        return sizeBytes == 0UL && crcText == F("-");
    }
    present = state == F("PRESENT");
    return present && parseHex32(crcText, crc32);
}

bool parseApplyPreflightEntry(const String& source,
                              String& remoteName,
                              String& targetPath,
                              uint32_t& stagedBytes,
                              uint32_t& stagedCrc,
                              bool& previousPresent,
                              uint32_t& previousBytes,
                              uint32_t& previousCrc)
{
    String line = source;
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    if (line.length() == 0U || line.length() > 500U) return false;
    int separators[6] = {-1, -1, -1, -1, -1, -1};
    int cursor = -1;
    for (uint8_t i = 0U; i < 6U; ++i)
    {
        cursor = line.indexOf('\t', cursor + 1);
        if (cursor < 0) return false;
        separators[i] = cursor;
    }
    if (line.indexOf('\t', separators[5] + 1) >= 0) return false;
    remoteName = line.substring(0U, static_cast<unsigned>(separators[0]));
    targetPath = line.substring(static_cast<unsigned>(separators[0] + 1),
                                static_cast<unsigned>(separators[1]));
    const String stagedSizeText = line.substring(
        static_cast<unsigned>(separators[1] + 1),
        static_cast<unsigned>(separators[2]));
    const String stagedCrcText = line.substring(
        static_cast<unsigned>(separators[2] + 1),
        static_cast<unsigned>(separators[3]));
    const String state = line.substring(
        static_cast<unsigned>(separators[3] + 1),
        static_cast<unsigned>(separators[4]));
    const String previousSizeText = line.substring(
        static_cast<unsigned>(separators[4] + 1),
        static_cast<unsigned>(separators[5]));
    const String previousCrcText = line.substring(
        static_cast<unsigned>(separators[5] + 1));
    if (!parseCanonicalUnsigned(stagedSizeText, 0UL, 536870912UL,
                                stagedBytes) ||
        !parseHex32(stagedCrcText, stagedCrc) ||
        !parseCanonicalUnsigned(previousSizeText, 0UL, 1073741824UL,
                                previousBytes)) return false;
    const bool safeNames = remoteName.length() > 0U &&
                           remoteName.length() <= 180U &&
                           remoteName.indexOf('/') < 0 &&
                           remoteName.indexOf("..") < 0 &&
                           targetPath.startsWith("/data/") &&
                           targetPath.length() <= 160U &&
                           targetPath.indexOf("..") < 0 &&
                           targetPath.indexOf('\t') < 0 &&
                           targetPath.indexOf('\r') < 0 &&
                           targetPath.indexOf('\n') < 0;
    if (!safeNames) return false;
    if (state == F("MISSING"))
    {
        previousPresent = false;
        previousCrc = 0UL;
        return previousBytes == 0UL && previousCrcText == F("-");
    }
    previousPresent = state == F("PRESENT");
    return previousPresent && parseHex32(previousCrcText, previousCrc);
}

uint32_t updateCrc32(uint32_t crc, const uint8_t* data, size_t length)
{
    for (size_t i = 0U; i < length; ++i)
    {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
            crc = (crc >> 1U) ^ (0xEDB88320UL &
                                 (0UL - (crc & 1UL)));
    }
    return crc;
}

bool readFtpReply(WiFiClient& client, uint16_t& code)
{
    code = 0U;
    const uint32_t deadline = millis() + 3500UL;
    String line;
    while (static_cast<int32_t>(deadline - millis()) > 0)
    {
        while (client.available())
        {
            const char ch = static_cast<char>(client.read());
            if (ch == '\r') continue;
            if (ch != '\n')
            {
                if (line.length() >= 255U) return false;
                line += ch;
                continue;
            }
            if (line.length() >= 4U && isDigit(line[0]) && isDigit(line[1]) &&
                isDigit(line[2]) && line[3] == ' ')
            {
                code = static_cast<uint16_t>((line[0] - '0') * 100 +
                                             (line[1] - '0') * 10 +
                                             (line[2] - '0'));
                return true;
            }
            line = String();
        }
        delay(1U);
    }
    return false;
}

bool sendFtpCommand(WiFiClient& client,
                    const String& command,
                    uint16_t& responseCode)
{
    if (client.print(command) != command.length() ||
        client.print("\r\n") != 2U)
    {
        return false;
    }
    return readFtpReply(client, responseCode);
}
}

RemoteBackupWeb::RemoteBackupWeb(WebServer& server,
                                 fs::FS& storage,
                                 RemoteBackupSettingsStore& settingsStore,
                                 RtcClock& rtcClock)
    : m_server(server),
      m_storage(storage),
      m_settingsStore(settingsStore),
      m_rtcClock(rtcClock),
      m_transfer(storage) {}

void RemoteBackupWeb::begin()
{
    m_server.on("/api/backup/remote/configuration", HTTP_GET,
                [this]() { handleGetConfiguration(); });
    m_server.on("/api/backup/remote/configuration", HTTP_POST,
                [this]() { handleSetConfiguration(); });
    m_server.on("/api/backup/remote/test", HTTP_POST,
                [this]() { handleTestConnection(); });
    m_server.on("/api/backup/remote/upload", HTTP_POST,
                [this]() { handleStartUpload(); });
    m_server.on("/api/backup/remote/status", HTTP_GET,
                [this]() { handleUploadStatus(); });
    m_server.on("/api/backup/remote/batch", HTTP_POST,
                [this]() { handleStartBatch(); });
    m_server.on("/api/backup/remote/retention", HTTP_POST,
                [this]() { handleStartRetention(); });
    m_server.on("/api/backup/remote/batch-status", HTTP_GET,
                [this]() { handleBatchStatus(); });
    m_server.on("/api/backup/remote/inspection", HTTP_POST,
                [this]() { handleStartInspection(); });
    m_server.on("/api/backup/remote/inspection-status", HTTP_GET,
                [this]() { handleInspectionStatus(); });
    m_server.on("/api/backup/remote/staging", HTTP_POST,
                [this]() { handleStartStaging(); });
    m_server.on("/api/backup/remote/staging-status", HTTP_GET,
                [this]() { handleStagingStatus(); });
    m_server.on("/api/backup/remote/staging", HTTP_DELETE,
                [this]() { handleDiscardStaging(); });
    m_server.on("/api/backup/remote/restore-plan", HTTP_POST,
                [this]() { handleStartRestorePlan(); });
    m_server.on("/api/backup/remote/restore-plan-status", HTTP_GET,
                [this]() { handleRestorePlanStatus(); });
    m_server.on("/api/backup/remote/rollback-snapshot", HTTP_POST,
                [this]() { handleStartRollbackSnapshot(); });
    m_server.on("/api/backup/remote/rollback-snapshot-status", HTTP_GET,
                [this]() { handleRollbackSnapshotStatus(); });
    m_server.on("/api/backup/remote/apply-preflight", HTTP_POST,
                [this]() { handleStartApplyPreflight(); });
    m_server.on("/api/backup/remote/apply-preflight-status", HTTP_GET,
                [this]() { handleApplyPreflightStatus(); });
    m_server.on("/api/backup/remote/apply", HTTP_POST,
                [this]() { handleStartApply(); });
    m_server.on("/api/backup/remote/apply-status", HTTP_GET,
                [this]() { handleApplyStatus(); });
}

void RemoteBackupWeb::update(uint32_t nowMs)
{
    m_transfer.update(nowMs);
    if (m_applyStage == ApplyStage::Entries)
    {
        if (!processNextApplyEntry() &&
            !beginApplyRollback("restore_apply_entry_failed"))
            failApply("restore_apply_rollback_start_failed");
        return;
    }
    if (m_applyStage == ApplyStage::Copying ||
        m_applyStage == ApplyStage::RollbackCopying)
    {
        if (!continueApplyCopy())
        {
            if (m_applyStage == ApplyStage::Copying &&
                beginApplyRollback("restore_apply_copy_failed")) return;
            failApply("restore_apply_rollback_copy_failed");
        }
        return;
    }
    if (m_applyStage == ApplyStage::Verifying ||
        m_applyStage == ApplyStage::RollbackVerifying)
    {
        if (!continueApplyVerification())
        {
            if (m_applyStage == ApplyStage::Verifying &&
                beginApplyRollback("restore_apply_verification_failed"))
                return;
            failApply("restore_apply_rollback_verification_failed");
        }
        return;
    }
    if (m_applyStage == ApplyStage::RollbackEntries)
    {
        if (!processNextApplyRollbackEntry())
            failApply("restore_apply_rollback_entry_failed");
        return;
    }
    if (m_applyPreflightStage == ApplyPreflightStage::Entries)
    {
        if (!processNextApplyPreflightEntry())
            failApplyPreflight("apply_preflight_entry_failed");
        return;
    }
    if (m_applyPreflightStage == ApplyPreflightStage::CurrentFile ||
        m_applyPreflightStage == ApplyPreflightStage::RollbackFile ||
        m_applyPreflightStage == ApplyPreflightStage::StagedFile)
    {
        if (!continueApplyPreflightCrc())
            failApplyPreflight("apply_preflight_crc_failed");
        return;
    }
    if (m_rollbackStage == RollbackStage::PlanFiles)
    {
        if (!processNextRollbackEntry())
            failRollbackSnapshot("rollback_plan_entry_failed");
        return;
    }
    if (m_rollbackStage == RollbackStage::Copying)
    {
        if (!continueRollbackCopy())
            failRollbackSnapshot("rollback_copy_failed");
        return;
    }
    if (m_rollbackStage == RollbackStage::Verifying)
    {
        if (!continueRollbackVerification())
            failRollbackSnapshot("rollback_verification_failed");
        return;
    }
    if (m_restorePlanStage == RestorePlanStage::Files)
    {
        if (!processNextRestorePlanEntry())
            failRestorePlan("restore_plan_entry_failed");
        return;
    }
    if (m_stagingStage == StagingStage::Files)
    {
        if (m_transfer.active()) return;
        if (!m_transfer.succeeded() ||
            m_transfer.bytesTotal() != m_stagingExpectedBytes)
        {
            failStaging(m_transfer.succeeded()
                            ? "staging_file_size_mismatch"
                            : m_transfer.error().c_str());
            return;
        }
        ++m_stagingFilesCompleted;
        m_stagingBytesCompleted += m_stagingExpectedBytes;
        if (!startNextStagingFile())
            failStaging("staging_next_file_failed");
        return;
    }
    if (m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files)
    {
        if (m_transfer.active()) return;
        if (!m_transfer.succeeded())
        {
            failInspection(m_transfer.error().c_str());
            return;
        }
        if (m_inspectionStage == InspectionStage::Manifest)
        {
            uint32_t dataFiles = 0UL;
            if (!validateInspectionManifest(dataFiles))
            {
                failInspection("inspection_manifest_invalid");
                return;
            }
            m_inspectionDataFiles = dataFiles;
            m_inspectionStage = InspectionStage::Marker;
            const String markerName = String(F("cm-b")) +
                                      m_inspectionBatchId +
                                      F("-COMPLETE.txt");
            if (!m_transfer.startDownload(m_batchSettings,
                                          F("inspection-complete"),
                                          markerName,
                                          InspectionMarkerPath,
                                          512UL))
                failInspection("inspection_marker_start_failed");
            return;
        }
        if (m_inspectionStage == InspectionStage::Files)
        {
            if (m_inspectionExpectedSizeValid &&
                m_transfer.bytesTotal() != m_inspectionExpectedBytes)
            {
                failInspection("inspection_file_size_mismatch");
                return;
            }
            ++m_inspectionFilesVerified;
            if (!startNextInspectionFile())
                failInspection("inspection_file_probe_failed");
            return;
        }
        if (!validateInspectionMarker())
        {
            failInspection("inspection_complete_invalid");
            return;
        }
        m_inspectionFilesVerified = 0UL;
        m_inspectionStage = InspectionStage::Files;
        if (!startNextInspectionFile())
            failInspection("inspection_file_probe_failed");
        return;
    }
    if (m_batchStage != BatchStage::MainFiles &&
        m_batchStage != BatchStage::SessionFiles &&
        m_batchStage != BatchStage::Manifest &&
        m_batchStage != BatchStage::Marker &&
        m_batchStage != BatchStage::Retention)
    {
        updateSchedule(nowMs);
        return;
    }
    if (m_transfer.active()) return;
    if (!m_transfer.succeeded())
    {
        if (m_batchStage == BatchStage::Retention)
            failRetention(m_transfer.error().c_str());
        else
            failBatch(m_transfer.error().c_str());
        return;
    }
    if (m_batchStage == BatchStage::Retention)
    {
        ++m_retentionFilesDeleted;
        if (m_retentionIncomplete)
        {
            if (!startNextIncompleteDelete())
                failRetention("incomplete_manifest_cleanup_failed");
            return;
        }
        if (!m_retentionMarkerDeleted)
        {
            m_retentionMarkerDeleted = true;
            if (!startNextRetentionFile())
                failRetention("retention_manifest_read_failed");
        }
        else if (!startNextRetentionFile())
            failRetention("retention_manifest_read_failed");
        return;
    }
    ++m_batchFilesCompleted;
    if (m_batchStage == BatchStage::Manifest)
    {
        m_storage.remove(RemoteBatchManifestPath);
        m_batchStage = BatchStage::Marker;
        if (!startNextBatchFile()) failBatch("batch_marker_start_failed");
        return;
    }
    if (m_batchStage == BatchStage::Marker)
    {
        m_storage.remove(BatchMarkerPath);
        m_storage.remove(RemoteBatchManifestPath);
        if (!finalizeBatchManifest())
        {
            failBatch("batch_manifest_finalize_failed");
            return;
        }
        if (!startRetention())
            failRetention("retention_start_failed");
        return;
    }
    if (!startNextBatchFile() &&
        m_batchStage != BatchStage::Complete &&
        m_batchStage != BatchStage::Failed)
        failBatch("batch_next_file_failed");
}

void RemoteBackupWeb::setActivityProbe(
    BackupActivityGuard::RuntimeProbe activityProbe)
{
    m_transfer.setActivityProbe(activityProbe);
}

void RemoteBackupWeb::handleGetConfiguration()
{
    RemoteBackupSettings settings;
    bool configured = false;
    if (!m_settingsStore.load(settings, configured))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_settings_unavailable\"}");
        return;
    }

    String response;
    response.reserve(560U);
    response = F("{\"supported\":true,\"configured\":");
    response += configured ? F("true") : F("false");
    response += F(",\"enabled\":");
    response += configured && settings.enabled ? F("true") : F("false");
    response += F(",\"host\":\"");
    if (configured) response += settings.host;
    response += F("\",\"port\":"); response += configured ? settings.port : 21U;
    response += F(",\"username\":\"");
    if (configured) response += settings.username;
    response += F("\",\"password_configured\":");
    response += configured && settings.password.length() > 0U ? F("true") : F("false");
    response += F(",\"remote_directory\":\"");
    response += configured ? settings.remoteDirectory : String(F("/CoilMaster"));
    response += F("\",\"retention_count\":");
    response += configured ? settings.retentionCount : 7U;
    response += F(",\"scheduled_backup_supported\":true,\"schedule_enabled\":");
    response += configured && settings.scheduleEnabled ? F("true") : F("false");
    response += F(",\"schedule_hour\":");
    response += configured ? settings.scheduleHour : 2U;
    response += F(",\"schedule_minute\":");
    response += configured ? settings.scheduleMinute : 0U;
    response += F(",\"last_scheduled_date\":");
    if (configured && settings.lastScheduledDate > 0UL)
        response += settings.lastScheduledDate;
    else response += F("null");
    response += F(",\"schedule_state\":\""); response += m_scheduleState;
    response += F("\",\"transport\":\"FTP\",\"credentials_exposed\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleSetConfiguration()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    if (!m_settingsStore.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_settings_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("enabled") || !m_server.hasArg("host") ||
        !m_server.hasArg("port") || !m_server.hasArg("username") ||
        !m_server.hasArg("remote_directory") ||
        !m_server.hasArg("retention_count"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_fields_required\"}");
        return;
    }

    RemoteBackupSettings previous;
    bool configured = false;
    if (!m_settingsStore.load(previous, configured))
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_settings_unavailable\"}");
        return;
    }

    RemoteBackupSettings settings;
    settings.host = m_server.arg("host");
    settings.username = m_server.arg("username");
    settings.remoteDirectory = m_server.arg("remote_directory");
    if (!parseBoolean(m_server.arg("enabled"), settings.enabled))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_remote_backup_enabled\"}");
        return;
    }
    uint32_t port = 0UL, retention = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("port"), 1UL, 65535UL, port) ||
        !parseCanonicalUnsigned(m_server.arg("retention_count"), 1UL, 30UL,
                                retention))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_remote_backup_number\"}");
        return;
    }
    settings.port = static_cast<uint16_t>(port);
    settings.retentionCount = static_cast<uint8_t>(retention);
    settings.scheduleEnabled = configured ? previous.scheduleEnabled : false;
    settings.scheduleHour = configured ? previous.scheduleHour : 2U;
    settings.scheduleMinute = configured ? previous.scheduleMinute : 0U;
    settings.lastScheduledDate = configured ? previous.lastScheduledDate : 0UL;
    const bool hasScheduleFields = m_server.hasArg("schedule_enabled") ||
                                   m_server.hasArg("schedule_hour") ||
                                   m_server.hasArg("schedule_minute");
    if (hasScheduleFields)
    {
        if (!m_server.hasArg("schedule_enabled") ||
            !m_server.hasArg("schedule_hour") ||
            !m_server.hasArg("schedule_minute"))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"remote_backup_schedule_fields_required\"}");
            return;
        }
        uint32_t scheduleHour = 0UL, scheduleMinute = 0UL;
        if (!parseBoolean(m_server.arg("schedule_enabled"),
                          settings.scheduleEnabled) ||
            !parseCanonicalUnsigned(m_server.arg("schedule_hour"),
                                    0UL, 23UL, scheduleHour) ||
            !parseCanonicalUnsigned(m_server.arg("schedule_minute"),
                                    0UL, 59UL, scheduleMinute))
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_remote_backup_schedule\"}");
            return;
        }
        settings.scheduleHour = static_cast<uint8_t>(scheduleHour);
        settings.scheduleMinute = static_cast<uint8_t>(scheduleMinute);
    }
    if (m_server.hasArg("password") && m_server.arg("password").length() > 0U)
        settings.password = m_server.arg("password");
    else if (configured)
        settings.password = previous.password;

    if (!RemoteBackupSettingsStore::valid(settings))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_remote_backup_settings\"}");
        return;
    }
    if (!m_settingsStore.save(settings))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_settings_write_failed\"}");
        return;
    }
    m_scheduleAttemptDate = 0UL;
    m_scheduleState = settings.scheduleEnabled ? F("WAITING_TIME") : F("DISABLED");
    m_server.send(200, "application/json; charset=utf-8",
                  "{\"saved\":true,\"credentials_exposed\":false}");
}

void RemoteBackupWeb::handleTestConnection()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"sta_not_connected\"}");
        return;
    }

    RemoteBackupSettings settings;
    bool configured = false;
    if (!m_settingsStore.load(settings, configured) || !configured)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_not_configured\"}");
        return;
    }

    WiFiClient client;
    client.setTimeout(3500UL);
    if (!client.connect(settings.host.c_str(), settings.port))
    {
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"ftp_connect_failed\"}");
        return;
    }

    uint16_t code = 0U;
    bool authenticated = readFtpReply(client, code) && code == 220U;
    if (authenticated)
    {
        authenticated = sendFtpCommand(client,
                                       String(F("USER ")) + settings.username,
                                       code) &&
                        (code == 230U || code == 331U);
    }
    if (authenticated && code == 331U)
    {
        authenticated = sendFtpCommand(client,
                                       String(F("PASS ")) + settings.password,
                                       code) && code == 230U;
    }
    bool directoryReady = false;
    if (authenticated)
    {
        directoryReady = sendFtpCommand(client,
                                        String(F("CWD ")) +
                                            settings.remoteDirectory,
                                        code) && code >= 200U && code < 300U;
        uint16_t ignored = 0U;
        sendFtpCommand(client, F("QUIT"), ignored);
    }
    client.stop();

    if (!authenticated)
    {
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"ftp_authentication_failed\"}");
        return;
    }
    if (!directoryReady)
    {
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"ftp_remote_directory_unavailable\"}");
        return;
    }
    m_server.send(200, "application/json; charset=utf-8",
                  "{\"connected\":true,\"authenticated\":true,\"remote_directory_ready\":true}");
}

void RemoteBackupWeb::handleStartUpload()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"backup_name_required\"}");
        return;
    }
    String stabilityReason;
    if (!BackupExportWeb::snapshotStable(m_storage, stabilityReason))
    {
        String response = F("{\"error\":\"snapshot_unstable\",\"reason\":\"");
        response += stabilityReason;
        response += F("\"}");
        m_server.send(409, "application/json; charset=utf-8", response);
        return;
    }
    String localPath, remoteName;
    const String logicalName = m_server.arg("name");
    if (!BackupExportWeb::resolveExportFile(logicalName,
                                            localPath,
                                            remoteName))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"backup_file_not_allowed\"}");
        return;
    }
    if (!m_storage.exists(localPath))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"backup_file_not_found\"}");
        return;
    }
    RemoteBackupSettings settings;
    bool configured = false;
    if (!m_settingsStore.load(settings, configured) || !configured ||
        !settings.enabled)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_not_enabled\"}");
        return;
    }
    if (!m_transfer.start(settings, logicalName, localPath, remoteName))
    {
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_start_failed\"}");
        return;
    }
    m_server.send(202, "application/json; charset=utf-8",
                  "{\"accepted\":true,\"state\":\"FTP_NEGOTIATION\"}");
}

void RemoteBackupWeb::handleUploadStatus()
{
    String response;
    response.reserve(260U);
    response = F("{\"state\":\""); response += m_transfer.stateName();
    response += F("\",\"active\":");
    response += m_transfer.active() ? F("true") : F("false");
    response += F(",\"succeeded\":");
    response += m_transfer.succeeded() ? F("true") : F("false");
    response += F(",\"name\":\""); response += m_transfer.logicalName();
    response += F("\",\"bytes_sent\":"); response += m_transfer.bytesSent();
    response += F(",\"bytes_total\":"); response += m_transfer.bytesTotal();
    response += F(",\"error\":");
    if (m_transfer.error().length() > 0U)
    {
        response += '"'; response += m_transfer.error(); response += '"';
    }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStartBatch()
{
    uint16_t statusCode = 500U;
    const char* error = "backup_batch_start_failed";
    if (!startFullBatch(false, statusCode, error))
    {
        String response = F("{\"error\":\"");
        response += error != nullptr ? error : "backup_batch_start_failed";
        response += F("\"}");
        m_server.send(statusCode, "application/json", response);
        return;
    }
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += m_batchId; response += '}';
    m_server.send(202, "application/json", response);
}

bool RemoteBackupWeb::startFullBatch(bool scheduled,
                                      uint16_t& statusCode,
                                      const char*& error)
{
    statusCode = 500U;
    error = "backup_batch_start_failed";
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        statusCode = 409U;
        error = "remote_backup_busy";
        return false;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    String stabilityReason;
    if (activity != BackupActivityCheck::Safe ||
        !BackupExportWeb::snapshotStable(m_storage, stabilityReason))
    {
        statusCode = activity == BackupActivityCheck::Unavailable ? 503U : 409U;
        error = "stable_snapshot_required";
        return false;
    }
    bool configured = false;
    if (!m_settingsStore.load(m_batchSettings, configured) || !configured ||
        !m_batchSettings.enabled || WiFi.status() != WL_CONNECTED)
    {
        statusCode = 409U;
        error = "remote_backup_not_ready";
        return false;
    }
    if (!allocateBatchId(m_batchId))
    {
        error = "backup_batch_id_failed";
        return false;
    }
    m_batchScheduled = scheduled;
    if (!scheduled) m_scheduledBatchDate = 0UL;
    m_batchMainIndex = 0U;
    m_batchAfterSessionId = 0UL;
    m_batchSessionId = 0UL;
    m_batchKindIndex = 0U;
    m_batchFilesCompleted = 0UL;
    m_batchError = String();
    m_retentionFilesDeleted = 0UL;
    m_incompleteBatchesDeleted = 0U;
    m_retentionSucceeded = true;
    m_retentionOnly = false;
    m_retentionError = String();
    if (!beginBatchManifest())
    {
        failBatch("batch_manifest_create_failed");
        error = "backup_batch_manifest_failed";
        return false;
    }
    m_batchStage = BatchStage::MainFiles;
    if (!startNextBatchFile())
    {
        failBatch("batch_first_file_failed");
        error = "backup_batch_start_failed";
        return false;
    }
    statusCode = 202U;
    error = nullptr;
    return true;
}

void RemoteBackupWeb::handleStartRetention()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    bool configured = false;
    if (!m_settingsStore.load(m_batchSettings, configured) || !configured ||
        !m_batchSettings.enabled || WiFi.status() != WL_CONNECTED)
    {
        m_server.send(409, "application/json",
                      "{\"error\":\"remote_backup_not_ready\"}");
        return;
    }

    m_batchId = 0UL;
    m_batchFilesCompleted = 0UL;
    m_batchError = String();
    m_retentionFilesDeleted = 0UL;
    m_incompleteBatchesDeleted = 0U;
    m_retentionSucceeded = true;
    m_retentionOnly = true;
    m_retentionError = String();
    if (!startRetention())
    {
        failRetention("retention_start_failed");
        m_server.send(500, "application/json",
                      "{\"error\":\"retention_start_failed\"}");
        return;
    }
    m_server.send(202, "application/json",
                  "{\"accepted\":true,\"operation\":\"RETENTION_ONLY\"}");
}

void RemoteBackupWeb::handleBatchStatus()
{
    const char* state = "IDLE";
    if (m_batchStage == BatchStage::MainFiles) state = "MAIN_FILES";
    else if (m_batchStage == BatchStage::SessionFiles) state = "SESSION_FILES";
    else if (m_batchStage == BatchStage::Manifest) state = "MANIFEST";
    else if (m_batchStage == BatchStage::Marker) state = "FINALIZING";
    else if (m_batchStage == BatchStage::Retention) state = "RETENTION";
    else if (m_batchStage == BatchStage::Complete) state = "COMPLETED";
    else if (m_batchStage == BatchStage::Failed) state = "FAILED";
    String response = F("{\"state\":\""); response += state;
    response += F("\",\"operation\":\"");
    response += m_retentionOnly ? F("RETENTION_ONLY") : F("FULL_BACKUP");
    response += F("\",\"active\":");
    response += (m_batchStage == BatchStage::MainFiles ||
                 m_batchStage == BatchStage::SessionFiles ||
                 m_batchStage == BatchStage::Manifest ||
                 m_batchStage == BatchStage::Marker ||
                 m_batchStage == BatchStage::Retention) ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_batchId > 0UL) response += m_batchId; else response += F("null");
    response += F(",\"scheduled\":");
    response += m_batchScheduled ? F("true") : F("false");
    response += F(",\"files_completed\":"); response += m_batchFilesCompleted;
    response += F(",\"current_name\":\""); response += m_transfer.logicalName();
    response += F("\",\"bytes_sent\":"); response += m_transfer.bytesSent();
    response += F(",\"bytes_total\":"); response += m_transfer.bytesTotal();
    response += F(",\"error\":");
    if (m_batchError.length() > 0U)
    { response += '"'; response += m_batchError; response += '"'; }
    else response += F("null");
    response += F(",\"retention_configured\":");
    response += m_batchSettings.retentionCount;
    response += F(",\"retention_files_deleted\":");
    response += m_retentionFilesDeleted;
    response += F(",\"incomplete_batches_deleted\":");
    response += m_incompleteBatchesDeleted;
    response += F(",\"retention_succeeded\":");
    response += m_retentionSucceeded ? F("true") : F("false");
    response += F(",\"retention_error\":");
    if (m_retentionError.length() > 0U)
    { response += '"'; response += m_retentionError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStartInspection()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    if (m_storage.exists(StagingDirectory) ||
        m_storage.exists(RollbackDirectory))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"staging_cleanup_required\"}");
        return;
    }
    if (!m_server.hasArg("batch_id"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"batch_id_required\"}");
        return;
    }
    uint32_t batchId = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("batch_id"), 1UL,
                                0xFFFFFFFFUL, batchId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_batch_id\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    bool configured = false;
    if (!m_settingsStore.load(m_batchSettings, configured) || !configured ||
        !m_batchSettings.enabled || WiFi.status() != WL_CONNECTED)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_not_ready\"}");
        return;
    }
    if (!m_storage.exists(InspectionDirectory) &&
        !m_storage.mkdir(InspectionDirectory))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"inspection_directory_failed\"}");
        return;
    }
    File directory = m_storage.open(InspectionDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"inspection_directory_invalid\"}");
        return;
    }
    directory.close();

    if (m_inspectionManifest) m_inspectionManifest.close();
    m_transfer.finishProbeSession();
    m_storage.remove(InspectionManifestPath);
    m_storage.remove(InspectionMarkerPath);
    m_inspectionBatchId = batchId;
    m_inspectionDataFiles = 0UL;
    m_inspectionTotalBytes = 0UL;
    m_inspectionFilesVerified = 0UL;
    m_inspectionExpectedBytes = 0UL;
    m_inspectionExpectedSizeValid = false;
    m_inspectionHasSizes = false;
    m_inspectionError = String();
    m_inspectionStage = InspectionStage::Manifest;
    const String manifestName = String(F("cm-b")) + batchId +
                                F("-MANIFEST.txt");
    if (!m_transfer.startDownload(m_batchSettings,
                                  F("inspection-manifest"),
                                  manifestName,
                                  InspectionManifestPath,
                                  131072UL))
    {
        failInspection("inspection_manifest_start_failed");
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"inspection_start_failed\"}");
        return;
    }
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"state\":\"MANIFEST\"}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleInspectionStatus()
{
    const char* state = "IDLE";
    if (m_inspectionStage == InspectionStage::Manifest) state = "MANIFEST";
    else if (m_inspectionStage == InspectionStage::Marker) state = "COMPLETE_MARKER";
    else if (m_inspectionStage == InspectionStage::Files) state = "FILES";
    else if (m_inspectionStage == InspectionStage::Complete) state = "VALID";
    else if (m_inspectionStage == InspectionStage::Failed) state = "FAILED";
    String response;
    response.reserve(320U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += (m_inspectionStage == InspectionStage::Manifest ||
                 m_inspectionStage == InspectionStage::Marker ||
                 m_inspectionStage == InspectionStage::Files)
                    ? F("true") : F("false");
    response += F(",\"valid\":");
    response += m_inspectionStage == InspectionStage::Complete
                    ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else response += F("null");
    response += F(",\"data_files\":"); response += m_inspectionDataFiles;
    response += F(",\"data_bytes\":"); response += m_inspectionTotalBytes;
    response += F(",\"files_verified\":"); response += m_inspectionFilesVerified;
    response += F(",\"sizes_verified\":");
    response += m_inspectionHasSizes ? F("true") : F("false");
    response += F(",\"bytes_received\":"); response += m_transfer.bytesSent();
    response += F(",\"bytes_total\":"); response += m_transfer.bytesTotal();
    response += F(",\"working_data_changed\":false,\"error\":");
    if (m_inspectionError.length() > 0U)
    { response += '"'; response += m_inspectionError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStartStaging()
{
    if (m_transfer.active() || m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive() ||
        m_inspectionStage != InspectionStage::Complete)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"validated_v2_inspection_required\"}");
        return;
    }
    if (!m_inspectionHasSizes || !m_server.hasArg("batch_id"))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"validated_v2_inspection_required\"}");
        return;
    }
    uint32_t batchId = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("batch_id"), 1UL,
                                0xFFFFFFFFUL, batchId) ||
        batchId != m_inspectionBatchId)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"inspection_batch_id_mismatch\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    bool configured = false;
    if (!m_settingsStore.load(m_batchSettings, configured) || !configured ||
        !m_batchSettings.enabled || WiFi.status() != WL_CONNECTED)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_not_ready\"}");
        return;
    }
    if (m_storage.exists(StagingDirectory))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"staging_already_exists\"}");
        return;
    }
    if (!m_storage.mkdir(StagingDirectory))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"staging_directory_failed\"}");
        return;
    }

    m_stagingFilesCompleted = 0UL;
    m_stagingExpectedBytes = 0UL;
    m_stagingBytesCompleted = 0UL;
    m_stagingError = String();
    m_stagingStage = StagingStage::Files;
    if (!startNextStagingFile())
    {
        failStaging("staging_first_file_failed");
        m_server.send(502, "application/json; charset=utf-8",
                      "{\"error\":\"staging_start_failed\"}");
        return;
    }
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"working_data_changed\":false}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStagingStatus()
{
    const char* state = "IDLE";
    if (m_stagingStage == StagingStage::Files) state = "FILES";
    else if (m_stagingStage == StagingStage::Complete) state = "STAGED";
    else if (m_stagingStage == StagingStage::Failed) state = "FAILED";
    else if (m_storage.exists(StagingDirectory)) state = "STALE";
    String response;
    response.reserve(360U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += m_stagingStage == StagingStage::Files ? F("true") : F("false");
    response += F(",\"ready\":");
    response += m_stagingStage == StagingStage::Complete ? F("true") : F("false");
    response += F(",\"staging_present\":");
    response += m_storage.exists(StagingDirectory) ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else response += F("null");
    response += F(",\"files_completed\":"); response += m_stagingFilesCompleted;
    response += F(",\"files_total\":"); response += m_inspectionDataFiles;
    response += F(",\"bytes_completed\":"); response += m_stagingBytesCompleted;
    response += F(",\"bytes_total\":"); response += m_inspectionTotalBytes;
    response += F(",\"working_data_changed\":false,\"restore_enabled\":false,\"error\":");
    if (m_stagingError.length() > 0U)
    { response += '"'; response += m_stagingError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleDiscardStaging()
{
    if (m_transfer.active() || m_stagingStage == StagingStage::Files ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (!clearRollbackDirectory() || !clearStagingDirectory())
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"staging_cleanup_failed\"}");
        return;
    }
    m_stagingStage = StagingStage::Idle;
    m_stagingFilesCompleted = 0UL;
    m_stagingBytesCompleted = 0UL;
    m_stagingError = String();
    m_restorePlanStage = RestorePlanStage::Idle;
    m_restorePlanFiles = 0UL;
    m_restorePlanBytes = 0UL;
    m_restorePlanError = String();
    m_rollbackStage = RollbackStage::Idle;
    m_rollbackFilesProcessed = 0UL;
    m_rollbackFilesPresent = 0UL;
    m_rollbackFilesMissing = 0UL;
    m_rollbackBytesCopied = 0UL;
    m_rollbackError = String();
    m_applyPreflightStage = ApplyPreflightStage::Idle;
    m_applyPreflightFiles = 0UL;
    m_applyPreflightBytes = 0UL;
    m_applyPreflightInputBytes = 0UL;
    m_applyPreflightInputExpected = 0UL;
    m_applyPreflightError = String();
    m_applyStage = ApplyStage::Idle;
    m_applyFiles = 0UL;
    m_applyRestoredFiles = 0UL;
    m_applyBytes = 0UL;
    m_applyCopiedBytes = 0UL;
    m_applyExpectedBytes = 0UL;
    m_applyError = String();
    m_applyRollbackReason = String();
    m_server.send(200, "application/json; charset=utf-8",
                  "{\"discarded\":true,\"working_data_changed\":false}");
}

void RemoteBackupWeb::handleStartRestorePlan()
{
    if (m_transfer.active() || m_stagingStage != StagingStage::Complete ||
        m_inspectionStage != InspectionStage::Complete ||
        !m_inspectionHasSizes ||
        m_restorePlanStage == RestorePlanStage::Files || rollbackActive() ||
        applyPreflightActive() || applyActive())
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"completed_staging_required\"}");
        return;
    }
    if (!m_server.hasArg("batch_id"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"batch_id_required\"}");
        return;
    }
    uint32_t batchId = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("batch_id"), 1UL,
                                0xFFFFFFFFUL, batchId) ||
        batchId != m_inspectionBatchId)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"staging_batch_id_mismatch\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (!validateStagingMarker() ||
        !m_storage.exists(InspectionManifestPath))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"staging_metadata_missing\"}");
        return;
    }
    m_storage.remove(RestorePlanTempPath);
    m_storage.remove(RestorePlanPath);
    m_restorePlanManifest = m_storage.open(InspectionManifestPath, FILE_READ);
    m_restorePlanOutput = m_storage.open(RestorePlanTempPath, FILE_WRITE);
    if (!m_restorePlanManifest || m_restorePlanManifest.isDirectory() ||
        !m_restorePlanOutput || m_restorePlanOutput.isDirectory())
    {
        failRestorePlan("restore_plan_open_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"restore_plan_open_failed\"}");
        return;
    }
    String ignored;
    for (uint8_t i = 0U; i < 3U; ++i)
    {
        if (!readInspectionLine(m_restorePlanManifest, ignored))
        {
            failRestorePlan("restore_plan_manifest_invalid");
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"restore_plan_manifest_invalid\"}");
            return;
        }
    }
    String header = F("COILMASTER_RESTORE_PLAN_V1\nbatch_id=");
    header += batchId;
    header += F("\nfiles="); header += m_inspectionDataFiles;
    header += F("\nbytes="); header += m_inspectionTotalBytes;
    header += F("\napply_enabled=0\n\n");
    if (m_restorePlanOutput.print(header) != header.length())
    {
        failRestorePlan("restore_plan_write_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"restore_plan_write_failed\"}");
        return;
    }
    m_restorePlanFiles = 0UL;
    m_restorePlanBytes = 0UL;
    m_restorePlanError = String();
    m_restorePlanStage = RestorePlanStage::Files;
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"apply_enabled\":false,\"working_data_changed\":false}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleRestorePlanStatus()
{
    const char* state = "IDLE";
    if (m_restorePlanStage == RestorePlanStage::Files) state = "PLANNING";
    else if (m_restorePlanStage == RestorePlanStage::Complete) state = "VALID";
    else if (m_restorePlanStage == RestorePlanStage::Failed) state = "FAILED";
    else if (m_storage.exists(RestorePlanPath)) state = "STALE";
    String response;
    response.reserve(300U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += m_restorePlanStage == RestorePlanStage::Files
                    ? F("true") : F("false");
    response += F(",\"valid\":");
    response += m_restorePlanStage == RestorePlanStage::Complete
                    ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else response += F("null");
    response += F(",\"files_planned\":"); response += m_restorePlanFiles;
    response += F(",\"files_total\":"); response += m_inspectionDataFiles;
    response += F(",\"bytes_planned\":"); response += m_restorePlanBytes;
    response += F(",\"bytes_total\":"); response += m_inspectionTotalBytes;
    response += F(",\"apply_enabled\":false,\"working_data_changed\":false,\"error\":");
    if (m_restorePlanError.length() > 0U)
    { response += '"'; response += m_restorePlanError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStartRollbackSnapshot()
{
    if (m_transfer.active() || rollbackActive() || applyPreflightActive() ||
        applyActive() ||
        m_stagingStage != StagingStage::Complete ||
        m_restorePlanStage != RestorePlanStage::Complete ||
        !m_inspectionHasSizes)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"validated_restore_plan_required\"}");
        return;
    }
    if (!m_server.hasArg("batch_id"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"batch_id_required\"}");
        return;
    }
    uint32_t batchId = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("batch_id"), 1UL,
                                0xFFFFFFFFUL, batchId) ||
        batchId != m_inspectionBatchId)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"restore_plan_batch_id_mismatch\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (!validateStagingMarker() || !m_storage.exists(RestorePlanPath))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"restore_plan_metadata_missing\"}");
        return;
    }
    if (m_storage.exists(RollbackDirectory))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"rollback_cleanup_required\"}");
        return;
    }

    m_rollbackPlan = m_storage.open(RestorePlanPath, FILE_READ);
    if (!m_rollbackPlan || m_rollbackPlan.isDirectory())
    {
        if (m_rollbackPlan) m_rollbackPlan.close();
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"restore_plan_open_failed\"}");
        return;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(m_rollbackPlan, line) &&
                 line == F("COILMASTER_RESTORE_PLAN_V1");
    if (valid)
        valid = readInspectionLine(m_rollbackPlan, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == batchId;
    if (valid)
        valid = readInspectionLine(m_rollbackPlan, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(m_rollbackPlan, line) &&
                line.startsWith("bytes=") &&
                parseCanonicalUnsigned(line.substring(6), 0UL,
                                       1073741824UL, parsed) &&
                parsed == m_inspectionTotalBytes;
    if (valid)
        valid = readInspectionLine(m_rollbackPlan, line) &&
                line == F("apply_enabled=0") &&
                readInspectionLine(m_rollbackPlan, line) && line.length() == 0U;
    if (!valid || !m_storage.mkdir(RollbackDirectory))
    {
        m_rollbackPlan.close();
        m_server.send(500, "application/json; charset=utf-8",
                      valid
                          ? "{\"error\":\"rollback_directory_failed\"}"
                          : "{\"error\":\"restore_plan_invalid\"}");
        return;
    }

    m_rollbackManifest = m_storage.open(RollbackManifestTempPath, FILE_WRITE);
    if (!m_rollbackManifest || m_rollbackManifest.isDirectory())
    {
        if (m_rollbackManifest) m_rollbackManifest.close();
        failRollbackSnapshot("rollback_manifest_open_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"rollback_manifest_open_failed\"}");
        return;
    }
    String header = F("COILMASTER_ROLLBACK_FILES_V1\nbatch_id=");
    header += batchId;
    header += F("\nplan_files="); header += m_inspectionDataFiles;
    header += F("\nrestore_apply_enabled=0\n\n");
    if (m_rollbackManifest.print(header) != header.length())
    {
        failRollbackSnapshot("rollback_manifest_write_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"rollback_manifest_write_failed\"}");
        return;
    }
    m_rollbackFilesProcessed = 0UL;
    m_rollbackFilesPresent = 0UL;
    m_rollbackFilesMissing = 0UL;
    m_rollbackBytesCopied = 0UL;
    m_rollbackError = String();
    m_rollbackStage = RollbackStage::PlanFiles;
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"restore_apply_enabled\":false,\"working_data_changed\":false}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleRollbackSnapshotStatus()
{
    const char* state = "IDLE";
    if (m_rollbackStage == RollbackStage::PlanFiles) state = "FILES";
    else if (m_rollbackStage == RollbackStage::Copying) state = "COPYING";
    else if (m_rollbackStage == RollbackStage::Verifying) state = "VERIFYING";
    else if (m_rollbackStage == RollbackStage::Complete) state = "READY";
    else if (m_rollbackStage == RollbackStage::Failed) state = "FAILED";
    else if (m_storage.exists(RollbackDirectory)) state = "STALE";
    String response;
    response.reserve(420U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += rollbackActive() ? F("true") : F("false");
    response += F(",\"ready\":");
    response += m_rollbackStage == RollbackStage::Complete
                    ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else response += F("null");
    response += F(",\"files_processed\":"); response += m_rollbackFilesProcessed;
    response += F(",\"files_total\":"); response += m_inspectionDataFiles;
    response += F(",\"files_present\":"); response += m_rollbackFilesPresent;
    response += F(",\"files_missing\":"); response += m_rollbackFilesMissing;
    response += F(",\"bytes_copied\":"); response += m_rollbackBytesCopied;
    response += F(",\"current_bytes\":"); response += m_rollbackCurrentBytes;
    response += F(",\"current_total\":"); response += m_rollbackCurrentExpected;
    response += F(",\"restore_apply_enabled\":false,\"working_data_changed\":false,\"error\":");
    if (m_rollbackError.length() > 0U)
    { response += '"'; response += m_rollbackError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleStartApplyPreflight()
{
    if (m_transfer.active() ||
        m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Manifest ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention ||
        m_inspectionStage == InspectionStage::Manifest ||
        m_inspectionStage == InspectionStage::Marker ||
        m_inspectionStage == InspectionStage::Files ||
        rollbackActive() || applyPreflightActive() || applyActive() ||
        m_stagingStage != StagingStage::Complete ||
        m_restorePlanStage != RestorePlanStage::Complete ||
        m_rollbackStage != RollbackStage::Complete ||
        !m_inspectionHasSizes)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"rollback_snapshot_required\"}");
        return;
    }
    if (!m_server.hasArg("batch_id"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"batch_id_required\"}");
        return;
    }
    uint32_t batchId = 0UL;
    if (!parseCanonicalUnsigned(m_server.arg("batch_id"), 1UL,
                                0xFFFFFFFFUL, batchId) ||
        batchId != m_inspectionBatchId)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_batch_id_mismatch\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe)
    {
        m_server.send(activity == BackupActivityCheck::Busy ? 409 : 503,
                      "application/json; charset=utf-8",
                      activity == BackupActivityCheck::Busy
                          ? "{\"error\":\"active_winding\"}"
                          : "{\"error\":\"activity_state_unavailable\"}");
        return;
    }
    if (!validateStagingMarker() || !validateRollbackMarker() ||
        !m_storage.exists(RestorePlanPath) ||
        !m_storage.exists(RollbackManifestPath))
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_metadata_invalid\"}");
        return;
    }

    m_storage.remove(ApplyPreflightTempPath);
    m_storage.remove(ApplyPreflightPath);
    m_storage.remove(ApplyReadyMarkerPath);
    m_applyPreflightPlan = m_storage.open(RestorePlanPath, FILE_READ);
    m_applyPreflightRollback = m_storage.open(RollbackManifestPath, FILE_READ);
    if (!m_applyPreflightPlan || m_applyPreflightPlan.isDirectory() ||
        !m_applyPreflightRollback || m_applyPreflightRollback.isDirectory())
    {
        failApplyPreflight("apply_preflight_metadata_open_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_metadata_open_failed\"}");
        return;
    }

    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(m_applyPreflightPlan, line) &&
                 line == F("COILMASTER_RESTORE_PLAN_V1");
    if (valid)
        valid = readInspectionLine(m_applyPreflightPlan, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == batchId;
    if (valid)
        valid = readInspectionLine(m_applyPreflightPlan, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(m_applyPreflightPlan, line) &&
                line.startsWith("bytes=") &&
                parseCanonicalUnsigned(line.substring(6), 0UL,
                                       1073741824UL, parsed) &&
                parsed == m_inspectionTotalBytes;
    if (valid)
        valid = readInspectionLine(m_applyPreflightPlan, line) &&
                line == F("apply_enabled=0") &&
                readInspectionLine(m_applyPreflightPlan, line) &&
                line.length() == 0U;

    if (valid)
        valid = readInspectionLine(m_applyPreflightRollback, line) &&
                line == F("COILMASTER_ROLLBACK_FILES_V1");
    if (valid)
        valid = readInspectionLine(m_applyPreflightRollback, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == batchId;
    if (valid)
        valid = readInspectionLine(m_applyPreflightRollback, line) &&
                line.startsWith("plan_files=") &&
                parseCanonicalUnsigned(line.substring(11), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(m_applyPreflightRollback, line) &&
                line == F("restore_apply_enabled=0") &&
                readInspectionLine(m_applyPreflightRollback, line) &&
                line.length() == 0U;
    if (!valid)
    {
        failApplyPreflight("apply_preflight_metadata_invalid");
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_metadata_invalid\"}");
        return;
    }

    m_applyPreflightOutput = m_storage.open(ApplyPreflightTempPath, FILE_WRITE);
    if (!m_applyPreflightOutput || m_applyPreflightOutput.isDirectory())
    {
        failApplyPreflight("apply_preflight_output_open_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_output_open_failed\"}");
        return;
    }
    String header = F("COILMASTER_APPLY_PREFLIGHT_V1\nbatch_id=");
    header += batchId;
    header += F("\nfiles="); header += m_inspectionDataFiles;
    header += F("\nrestore_apply_enabled=0\nworking_data_changed=0\n\n");
    if (m_applyPreflightOutput.print(header) != header.length())
    {
        failApplyPreflight("apply_preflight_output_write_failed");
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_output_write_failed\"}");
        return;
    }
    m_applyPreflightFiles = 0UL;
    m_applyPreflightBytes = 0UL;
    m_applyPreflightError = String();
    m_applyPreflightStage = ApplyPreflightStage::Entries;
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"restore_apply_enabled\":false,\"working_data_changed\":false}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleApplyPreflightStatus()
{
    const char* state = "IDLE";
    if (m_applyPreflightStage == ApplyPreflightStage::Entries) state = "FILES";
    else if (m_applyPreflightStage == ApplyPreflightStage::CurrentFile)
        state = "CURRENT";
    else if (m_applyPreflightStage == ApplyPreflightStage::RollbackFile)
        state = "ROLLBACK";
    else if (m_applyPreflightStage == ApplyPreflightStage::StagedFile)
        state = "STAGED";
    else if (m_applyPreflightStage == ApplyPreflightStage::Complete)
        state = "READY";
    else if (m_applyPreflightStage == ApplyPreflightStage::Failed)
        state = "FAILED";
    else if (m_storage.exists(ApplyPreflightPath) ||
             m_storage.exists(ApplyReadyMarkerPath)) state = "STALE";
    String response;
    response.reserve(360U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += applyPreflightActive() ? F("true") : F("false");
    response += F(",\"ready\":");
    response += m_applyPreflightStage == ApplyPreflightStage::Complete
                    ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else respon…869 tokens truncated…son; charset=utf-8",
                      "{\"error\":\"apply_metadata_open_failed\"}");
        return;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(m_applyManifest, line) &&
                 line == F("COILMASTER_APPLY_PREFLIGHT_V1");
    if (valid)
        valid = readInspectionLine(m_applyManifest, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == batchId;
    if (valid)
        valid = readInspectionLine(m_applyManifest, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(m_applyManifest, line) &&
                line == F("restore_apply_enabled=0");
    if (valid)
        valid = readInspectionLine(m_applyManifest, line) &&
                line == F("working_data_changed=0") &&
                readInspectionLine(m_applyManifest, line) &&
                line.length() == 0U;
    if (!valid)
    {
        m_applyManifest.close();
        m_applyJournal.close();
        m_storage.remove(ApplyJournalPath);
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"apply_preflight_invalid\"}");
        return;
    }
    String header = F("COILMASTER_APPLY_JOURNAL_V1\nbatch_id=");
    header += batchId;
    header += F("\nfiles="); header += m_inspectionDataFiles;
    header += F("\nstate=RUNNING\n\n");
    if (m_applyJournal.print(header) != header.length())
    {
        m_applyManifest.close();
        m_applyJournal.close();
        m_storage.remove(ApplyJournalPath);
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"apply_journal_write_failed\"}");
        return;
    }
    m_applyJournal.flush();
    m_applyFiles = 0UL;
    m_applyRestoredFiles = 0UL;
    m_applyBytes = 0UL;
    m_applyCopiedBytes = 0UL;
    m_applyExpectedBytes = 0UL;
    m_applyError = String();
    m_applyRollbackReason = String();
    m_applyStage = ApplyStage::Entries;
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += batchId;
    response += F(",\"operator_confirmed\":true,\"working_data_changed\":false}");
    m_server.send(202, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleApplyStatus()
{
    const char* state = "IDLE";
    if (m_applyStage == ApplyStage::Entries) state = "APPLYING";
    else if (m_applyStage == ApplyStage::Copying) state = "COPYING";
    else if (m_applyStage == ApplyStage::Verifying) state = "VERIFYING";
    else if (m_applyStage == ApplyStage::RollbackEntries ||
             m_applyStage == ApplyStage::RollbackCopying ||
             m_applyStage == ApplyStage::RollbackVerifying) state = "ROLLING_BACK";
    else if (m_applyStage == ApplyStage::Complete) state = "APPLIED";
    else if (m_applyStage == ApplyStage::RolledBack) state = "ROLLED_BACK";
    else if (m_applyStage == ApplyStage::Failed) state = "FAILED";
    else if (m_storage.exists(ApplyJournalPath) ||
             m_storage.exists(ApplyResultMarkerPath)) state = "STALE";
    String response;
    response.reserve(420U);
    response = F("{\"state\":\""); response += state;
    response += F("\",\"active\":");
    response += applyActive() ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_inspectionBatchId > 0UL) response += m_inspectionBatchId;
    else response += F("null");
    response += F(",\"files_applied\":"); response += m_applyFiles;
    response += F(",\"files_total\":"); response += m_inspectionDataFiles;
    response += F(",\"files_restored\":"); response += m_applyRestoredFiles;
    response += F(",\"bytes_applied\":"); response += m_applyBytes;
    response += F(",\"current_bytes\":"); response += m_applyCopiedBytes;
    response += F(",\"current_total\":"); response += m_applyExpectedBytes;
    response += F(",\"working_data_changed\":");
    response += (m_applyStage == ApplyStage::RolledBack)
                    ? F("false")
                    : ((m_applyFiles > 0UL ||
                        m_applyStage == ApplyStage::Complete)
                           ? F("true") : F("false"));
    response += F(",\"restore_applied\":");
    response += m_applyStage == ApplyStage::Complete ? F("true") : F("false");
    response += F(",\"reboot_required\":");
    response += m_applyStage == ApplyStage::Complete ? F("true") : F("false");
    response += F(",\"rollback_reason\":");
    if (m_applyRollbackReason.length() > 0U)
    { response += '"'; response += m_applyRollbackReason; response += '"'; }
    else response += F("null");
    response += F(",\"error\":");
    if (m_applyError.length() > 0U)
    { response += '"'; response += m_applyError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
}

bool RemoteBackupWeb::validateInspectionManifest(uint32_t& dataFiles)
{
    dataFiles = 0UL;
    File manifest = m_storage.open(InspectionManifestPath, FILE_READ);
    if (!manifest || manifest.isDirectory())
    {
        if (manifest) manifest.close();
        return false;
    }
    String line;
    bool valid = readInspectionLine(manifest, line) &&
                 (line == F("COILMASTER_BACKUP_MANIFEST_V1") ||
                  line == F("COILMASTER_BACKUP_MANIFEST_V2"));
    m_inspectionHasSizes = valid &&
                           line == F("COILMASTER_BACKUP_MANIFEST_V2");
    if (valid)
    {
        valid = readInspectionLine(manifest, line) &&
                line.startsWith("batch_id=");
        uint32_t parsed = 0UL;
        if (valid)
            valid = parseCanonicalUnsigned(line.substring(9), 1UL,
                                           0xFFFFFFFFUL, parsed) &&
                    parsed == m_inspectionBatchId;
    }
    if (valid)
    {
        valid = readInspectionLine(manifest, line) &&
                line.startsWith("data_files=");
        if (valid)
            valid = parseCanonicalUnsigned(line.substring(11), 1UL,
                                           4096UL, dataFiles);
    }

    const String prefix = String(F("cm-b")) + m_inspectionBatchId + '-';
    const String manifestName = prefix + F("MANIFEST.txt");
    const String markerName = prefix + F("COMPLETE.txt");
    uint32_t counted = 0UL;
    while (valid && manifest.available())
    {
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        valid = readInspectionLine(manifest, line) &&
                parseManagedManifestEntry(line, m_inspectionBatchId,
                                          remoteName, hasExpectedBytes,
                                          expectedBytes) &&
                hasExpectedBytes == m_inspectionHasSizes &&
                remoteName != manifestName && remoteName != markerName;
        if (valid && m_inspectionHasSizes)
        {
            valid = expectedBytes <= 536870912UL &&
                    expectedBytes <= 1073741824UL - m_inspectionTotalBytes;
            if (valid) m_inspectionTotalBytes += expectedBytes;
        }
        if (valid) ++counted;
    }
    manifest.close();
    return valid && counted == dataFiles;
}

bool RemoteBackupWeb::validateInspectionMarker() const
{
    File marker = m_storage.open(InspectionMarkerPath, FILE_READ);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String line;
    bool valid = readInspectionLine(marker, line) &&
                 line == F("COILMASTER_BACKUP_COMPLETE");
    uint32_t parsed = 0UL;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == m_inspectionBatchId;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files_before_marker=") &&
                parseCanonicalUnsigned(line.substring(20), 1UL,
                                       4097UL, parsed) &&
                parsed == m_inspectionDataFiles + 1UL;
    if (valid && marker.available()) valid = false;
    marker.close();
    return valid;
}

bool RemoteBackupWeb::startNextInspectionFile()
{
    if (!m_inspectionManifest)
    {
        m_inspectionManifest = m_storage.open(InspectionManifestPath, FILE_READ);
        if (!m_inspectionManifest || m_inspectionManifest.isDirectory())
        {
            if (m_inspectionManifest) m_inspectionManifest.close();
            return false;
        }
        String ignored;
        for (uint8_t i = 0U; i < 3U; ++i)
        {
            if (!readInspectionLine(m_inspectionManifest, ignored))
            {
                m_inspectionManifest.close();
                return false;
            }
        }
    }

    if (m_inspectionManifest.available())
    {
        const String entry = m_inspectionManifest.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_inspectionBatchId,
                                       remoteName, hasExpectedBytes,
                                       expectedBytes) ||
            hasExpectedBytes != m_inspectionHasSizes)
            return false;
        m_inspectionExpectedBytes = expectedBytes;
        m_inspectionExpectedSizeValid = hasExpectedBytes;
        return m_transfer.startSizeProbe(m_batchSettings,
                                         F("inspection-file"),
                                         remoteName);
    }

    m_inspectionManifest.close();
    m_transfer.finishProbeSession();
    if (m_inspectionFilesVerified != m_inspectionDataFiles) return false;
    m_inspectionStage = InspectionStage::Complete;
    return true;
}

void RemoteBackupWeb::failInspection(const char* reason)
{
    if (m_inspectionManifest) m_inspectionManifest.close();
    m_transfer.finishProbeSession();
    m_inspectionError = reason != nullptr ? reason : "inspection_failed";
    m_storage.remove(String(InspectionManifestPath) + F(".part"));
    m_storage.remove(String(InspectionMarkerPath) + F(".part"));
    m_storage.remove(InspectionManifestPath);
    m_storage.remove(InspectionMarkerPath);
    m_inspectionStage = InspectionStage::Failed;
}

bool RemoteBackupWeb::startNextStagingFile()
{
    if (!m_stagingManifest)
    {
        m_stagingManifest = m_storage.open(InspectionManifestPath, FILE_READ);
        if (!m_stagingManifest || m_stagingManifest.isDirectory())
        {
            if (m_stagingManifest) m_stagingManifest.close();
            return false;
        }
        String ignored;
        for (uint8_t i = 0U; i < 3U; ++i)
        {
            if (!readInspectionLine(m_stagingManifest, ignored))
            {
                m_stagingManifest.close();
                return false;
            }
        }
    }

    if (m_stagingManifest.available())
    {
        const String entry = m_stagingManifest.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_inspectionBatchId,
                                       remoteName, hasExpectedBytes,
                                       expectedBytes) ||
            !hasExpectedBytes || expectedBytes > 536870912UL)
            return false;
        m_stagingExpectedBytes = expectedBytes;
        const String localPath = String(StagingDirectory) + '/' + remoteName;
        if (m_storage.exists(localPath) ||
            m_storage.exists(localPath + F(".part"))) return false;
        return m_transfer.startDownload(m_batchSettings,
                                        F("staging-file"),
                                        remoteName,
                                        localPath,
                                        expectedBytes);
    }

    m_stagingManifest.close();
    if (m_stagingFilesCompleted != m_inspectionDataFiles ||
        m_stagingBytesCompleted != m_inspectionTotalBytes)
        return false;
    File marker = m_storage.open(StagingMarkerPath, FILE_WRITE);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String content = F("COILMASTER_RESTORE_STAGING_V1\nbatch_id=");
    content += m_inspectionBatchId;
    content += F("\nfiles="); content += m_stagingFilesCompleted;
    content += F("\nbytes="); content += m_stagingBytesCompleted;
    content += F("\nrestore_enabled=0\n");
    const bool written = marker.print(content) == content.length();
    marker.flush();
    marker.close();
    if (!written) return false;
    m_stagingStage = StagingStage::Complete;
    return true;
}

bool RemoteBackupWeb::clearStagingDirectory()
{
    if (!m_storage.exists(StagingDirectory)) return true;
    for (uint16_t removed = 0U; removed <= 4098U; ++removed)
    {
        File directory = m_storage.open(StagingDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return false;
        }
        File entry = directory.openNextFile();
        if (!entry)
        {
            directory.close();
            return m_storage.rmdir(StagingDirectory);
        }
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return false;
        }
        String name = entry.name();
        entry.close();
        directory.close();
        const int slash = name.lastIndexOf('/');
        if (slash >= 0)
            name = name.substring(static_cast<unsigned>(slash + 1));
        if (name.length() == 0U || name.length() > 200U ||
            name.indexOf('/') >= 0 || name.indexOf("..") >= 0 ||
            name.indexOf('\r') >= 0 || name.indexOf('\n') >= 0 ||
            !m_storage.remove(String(StagingDirectory) + '/' + name))
            return false;
    }
    return false;
}

void RemoteBackupWeb::failStaging(const char* reason)
{
    if (m_stagingManifest) m_stagingManifest.close();
    m_transfer.finishProbeSession();
    m_stagingError = reason != nullptr ? reason : "staging_failed";
    clearStagingDirectory();
    m_stagingStage = StagingStage::Failed;
}

bool RemoteBackupWeb::validateStagingMarker() const
{
    File marker = m_storage.open(StagingMarkerPath, FILE_READ);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(marker, line) &&
                 line == F("COILMASTER_RESTORE_STAGING_V1");
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == m_inspectionBatchId;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("bytes=") &&
                parseCanonicalUnsigned(line.substring(6), 0UL,
                                       1073741824UL, parsed) &&
                parsed == m_inspectionTotalBytes;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line == F("restore_enabled=0") && !marker.available();
    marker.close();
    return valid;
}

bool RemoteBackupWeb::processNextRestorePlanEntry()
{
    if (!m_restorePlanManifest || !m_restorePlanOutput) return false;
    if (m_restorePlanManifest.available())
    {
        const String entry = m_restorePlanManifest.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_inspectionBatchId,
                                       remoteName, hasExpectedBytes,
                                       expectedBytes) ||
            !hasExpectedBytes || expectedBytes > 536870912UL)
            return false;

        String logicalName;
        String targetPath;
        if (!BackupExportWeb::resolveRestoreTarget(m_inspectionBatchId,
                                                   remoteName,
                                                   logicalName,
                                                   targetPath))
            return false;
        const String stagedPath = String(StagingDirectory) + '/' + remoteName;
        File staged = m_storage.open(stagedPath, FILE_READ);
        if (!staged || staged.isDirectory())
        {
            if (staged) staged.close();
            return false;
        }
        const uint32_t stagedBytes = static_cast<uint32_t>(staged.size());
        staged.close();
        if (stagedBytes != expectedBytes ||
            expectedBytes > 1073741824UL - m_restorePlanBytes)
            return false;

        String line = remoteName;
        line += '\t'; line += expectedBytes;
        line += '\t'; line += logicalName;
        line += '\t'; line += targetPath;
        line += '\n';
        if (m_restorePlanOutput.print(line) != line.length()) return false;
        ++m_restorePlanFiles;
        m_restorePlanBytes += expectedBytes;
        return true;
    }

    m_restorePlanManifest.close();
    if (m_restorePlanFiles != m_inspectionDataFiles ||
        m_restorePlanBytes != m_inspectionTotalBytes)
        return false;
    m_restorePlanOutput.flush();
    m_restorePlanOutput.close();
    if (m_storage.exists(RestorePlanPath) &&
        !m_storage.remove(RestorePlanPath)) return false;
    if (!m_storage.rename(RestorePlanTempPath, RestorePlanPath)) return false;
    m_restorePlanStage = RestorePlanStage::Complete;
    return true;
}

void RemoteBackupWeb::failRestorePlan(const char* reason)
{
    if (m_restorePlanManifest) m_restorePlanManifest.close();
    if (m_restorePlanOutput) m_restorePlanOutput.close();
    m_storage.remove(RestorePlanTempPath);
    m_storage.remove(RestorePlanPath);
    m_restorePlanError = reason != nullptr ? reason : "restore_plan_failed";
    m_restorePlanStage = RestorePlanStage::Failed;
}

bool RemoteBackupWeb::processNextRollbackEntry()
{
    if (!m_rollbackPlan || !m_rollbackManifest) return false;
    if (!m_rollbackPlan.available()) return finalizeRollbackSnapshot();

    const String entry = m_rollbackPlan.readStringUntil('\n');
    String remoteName, logicalName, targetPath;
    uint32_t expectedBytes = 0UL;
    if (!parseRestorePlanEntry(entry, remoteName, expectedBytes,
                               logicalName, targetPath)) return false;
    String resolvedLogical, resolvedTarget;
    if (!BackupExportWeb::resolveRestoreTarget(m_inspectionBatchId,
                                               remoteName,
                                               resolvedLogical,
                                               resolvedTarget) ||
        resolvedLogical != logicalName || resolvedTarget != targetPath)
        return false;

    const String stagedPath = String(StagingDirectory) + '/' + remoteName;
    File staged = m_storage.open(stagedPath, FILE_READ);
    if (!staged || staged.isDirectory())
    {
        if (staged) staged.close();
        return false;
    }
    const uint32_t stagedBytes = static_cast<uint32_t>(staged.size());
    staged.close();
    if (stagedBytes != expectedBytes) return false;

    m_rollbackRemoteName = remoteName;
    m_rollbackTargetPath = targetPath;
    m_rollbackCurrentBytes = 0UL;
    m_rollbackCurrentExpected = 0UL;
    if (!m_storage.exists(targetPath))
    {
        if (!appendRollbackManifestEntry(false, 0UL, 0UL)) return false;
        ++m_rollbackFilesProcessed;
        ++m_rollbackFilesMissing;
        return true;
    }

    m_rollbackSource = m_storage.open(targetPath, FILE_READ);
    if (!m_rollbackSource || m_rollbackSource.isDirectory())
    {
        if (m_rollbackSource) m_rollbackSource.close();
        return false;
    }
    const size_t rawSize = m_rollbackSource.size();
    if (rawSize > 0xFFFFFFFFUL ||
        rawSize > 1073741824UL - m_rollbackBytesCopied)
    {
        m_rollbackSource.close();
        return false;
    }
    m_rollbackCurrentExpected = static_cast<uint32_t>(rawSize);
    const String finalPath = String(RollbackDirectory) + '/' + remoteName;
    m_rollbackPartPath = finalPath + F(".part");
    if (m_storage.exists(finalPath) || m_storage.exists(m_rollbackPartPath))
    {
        m_rollbackSource.close();
        return false;
    }
    m_rollbackCopy = m_storage.open(m_rollbackPartPath, FILE_WRITE);
    if (!m_rollbackCopy || m_rollbackCopy.isDirectory())
    {
        if (m_rollbackCopy) m_rollbackCopy.close();
        m_rollbackSource.close();
        return false;
    }
    m_rollbackSourceCrc = 0xFFFFFFFFUL;
    m_rollbackStage = RollbackStage::Copying;
    return true;
}

bool RemoteBackupWeb::continueRollbackCopy()
{
    if (!m_rollbackSource || !m_rollbackCopy ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    uint8_t buffer[1024];
    if (m_rollbackSource.available())
    {
        const size_t remaining = static_cast<size_t>(m_rollbackCurrentExpected -
                                                     m_rollbackCurrentBytes);
        const size_t requested = remaining < sizeof(buffer) ? remaining
                                                            : sizeof(buffer);
        if (requested == 0U) return false;
        const int read = m_rollbackSource.read(buffer, requested);
        if (read <= 0 || m_rollbackCopy.write(buffer, static_cast<size_t>(read)) !=
                         static_cast<size_t>(read)) return false;
        m_rollbackSourceCrc = updateCrc32(m_rollbackSourceCrc, buffer,
                                         static_cast<size_t>(read));
        m_rollbackCurrentBytes += static_cast<uint32_t>(read);
        return m_rollbackCurrentBytes <= m_rollbackCurrentExpected;
    }

    m_rollbackSource.close();
    m_rollbackCopy.flush();
    m_rollbackCopy.close();
    if (m_rollbackCurrentBytes != m_rollbackCurrentExpected) return false;
    File current = m_storage.open(m_rollbackTargetPath, FILE_READ);
    if (!current || current.isDirectory() ||
        static_cast<uint32_t>(current.size()) != m_rollbackCurrentExpected)
    {
        if (current) current.close();
        return false;
    }
    current.close();
    m_rollbackCopy = m_storage.open(m_rollbackPartPath, FILE_READ);
    if (!m_rollbackCopy || m_rollbackCopy.isDirectory())
    {
        if (m_rollbackCopy) m_rollbackCopy.close();
        return false;
    }
    m_rollbackVerifyCrc = 0xFFFFFFFFUL;
    m_rollbackVerifyBytes = 0UL;
    m_rollbackStage = RollbackStage::Verifying;
    return true;
}

bool RemoteBackupWeb::continueRollbackVerification()
{
    if (!m_rollbackCopy ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    uint8_t buffer[1024];
    if (m_rollbackCopy.available())
    {
        const int read = m_rollbackCopy.read(buffer, sizeof(buffer));
        if (read <= 0) return false;
        m_rollbackVerifyCrc = updateCrc32(m_rollbackVerifyCrc, buffer,
                                         static_cast<size_t>(read));
        m_rollbackVerifyBytes += static_cast<uint32_t>(read);
        return m_rollbackVerifyBytes <= m_rollbackCurrentExpected;
    }
    m_rollbackCopy.close();
    const uint32_t sourceCrc = m_rollbackSourceCrc ^ 0xFFFFFFFFUL;
    const uint32_t verifyCrc = m_rollbackVerifyCrc ^ 0xFFFFFFFFUL;
    if (m_rollbackVerifyBytes != m_rollbackCurrentExpected ||
        verifyCrc != sourceCrc) return false;
    const String finalPath = String(RollbackDirectory) + '/' +
                             m_rollbackRemoteName;
    if (!m_storage.rename(m_rollbackPartPath, finalPath) ||
        !appendRollbackManifestEntry(true, m_rollbackCurrentExpected,
                                     sourceCrc)) return false;
    ++m_rollbackFilesProcessed;
    ++m_rollbackFilesPresent;
    m_rollbackBytesCopied += m_rollbackCurrentExpected;
    m_rollbackCurrentBytes = 0UL;
    m_rollbackCurrentExpected = 0UL;
    m_rollbackPartPath = String();
    m_rollbackStage = RollbackStage::PlanFiles;
    return true;
}

bool RemoteBackupWeb::appendRollbackManifestEntry(bool present,
                                                   uint32_t sizeBytes,
                                                   uint32_t crc32)
{
    if (!m_rollbackManifest) return false;
    String line = m_rollbackRemoteName;
    line += '\t'; line += m_rollbackTargetPath;
    line += present ? F("\tPRESENT\t") : F("\tMISSING\t");
    line += sizeBytes;
    line += '\t';
    if (present)
    {
        String checksum(crc32, HEX);
        while (checksum.length() < 8U) checksum = String('0') + checksum;
        line += checksum;
    }
    else line += '-';
    line += '\n';
    const bool written = m_rollbackManifest.print(line) == line.length();
    m_rollbackManifest.flush();
    return written;
}

bool RemoteBackupWeb::finalizeRollbackSnapshot()
{
    if (m_rollbackPlan) m_rollbackPlan.close();
    if (!m_rollbackManifest ||
        m_rollbackFilesProcessed != m_inspectionDataFiles ||
        m_rollbackFilesPresent + m_rollbackFilesMissing !=
            m_rollbackFilesProcessed)
        return false;
    m_rollbackManifest.flush();
    m_rollbackManifest.close();
    if (!m_storage.rename(RollbackManifestTempPath, RollbackManifestPath))
        return false;
    File marker = m_storage.open(RollbackMarkerPath, FILE_WRITE);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String content = F("COILMASTER_ROLLBACK_SNAPSHOT_V1\nbatch_id=");
    content += m_inspectionBatchId;
    content += F("\nfiles_processed="); content += m_rollbackFilesProcessed;
    content += F("\nfiles_present="); content += m_rollbackFilesPresent;
    content += F("\nfiles_missing="); content += m_rollbackFilesMissing;
    content += F("\nbytes="); content += m_rollbackBytesCopied;
    content += F("\nrestore_apply_enabled=0\nrestore_applied=0\n");
    const bool written = marker.print(content) == content.length();
    marker.flush();
    marker.close();
    if (!written) return false;
    m_rollbackStage = RollbackStage::Complete;
    return true;
}

bool RemoteBackupWeb::clearRollbackDirectory()
{
    if (!m_storage.exists(RollbackDirectory)) return true;
    for (uint16_t removed = 0U; removed <= 4100U; ++removed)
    {
        File directory = m_storage.open(RollbackDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return false;
        }
        File entry = directory.openNextFile();
        if (!entry)
        {
            directory.close();
            return m_storage.rmdir(RollbackDirectory);
        }
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return false;
        }
        String name = entry.name();
        entry.close();
        directory.close();
        const int slash = name.lastIndexOf('/');
        if (slash >= 0)
            name = name.substring(static_cast<unsigned>(slash + 1));
        if (name.length() == 0U || name.length() > 200U ||
            name.indexOf('/') >= 0 || name.indexOf("..") >= 0 ||
            name.indexOf('\r') >= 0 || name.indexOf('\n') >= 0 ||
            !m_storage.remove(String(RollbackDirectory) + '/' + name))
            return false;
    }
    return false;
}

void RemoteBackupWeb::failRollbackSnapshot(const char* reason)
{
    if (m_rollbackPlan) m_rollbackPlan.close();
    if (m_rollbackManifest) m_rollbackManifest.close();
    if (m_rollbackSource) m_rollbackSource.close();
    if (m_rollbackCopy) m_rollbackCopy.close();
    m_rollbackError = reason != nullptr ? reason : "rollback_snapshot_failed";
    clearRollbackDirectory();
    m_rollbackStage = RollbackStage::Failed;
}

bool RemoteBackupWeb::rollbackActive() const
{
    return m_rollbackStage == RollbackStage::PlanFiles ||
           m_rollbackStage == RollbackStage::Copying ||
           m_rollbackStage == RollbackStage::Verifying;
}

bool RemoteBackupWeb::validateRollbackMarker() const
{
    File marker = m_storage.open(RollbackMarkerPath, FILE_READ);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(marker, line) &&
                 line == F("COILMASTER_ROLLBACK_SNAPSHOT_V1");
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == m_inspectionBatchId;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files_processed=") &&
                parseCanonicalUnsigned(line.substring(16), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_rollbackFilesProcessed &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files_present=") &&
                parseCanonicalUnsigned(line.substring(14), 0UL, 4096UL,
                                       parsed) &&
                parsed == m_rollbackFilesPresent;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files_missing=") &&
                parseCanonicalUnsigned(line.substring(14), 0UL, 4096UL,
                                       parsed) &&
                parsed == m_rollbackFilesMissing;
    if (valid)
        valid = m_rollbackFilesPresent + m_rollbackFilesMissing ==
                    m_rollbackFilesProcessed;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("bytes=") &&
                parseCanonicalUnsigned(line.substring(6), 0UL,
                                       1073741824UL, parsed) &&
                parsed == m_rollbackBytesCopied;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line == F("restore_apply_enabled=0");
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line == F("restore_applied=0") && !marker.available();
    marker.close();
    return valid;
}

bool RemoteBackupWeb::processNextApplyPreflightEntry()
{
    if (!m_applyPreflightPlan || !m_applyPreflightRollback ||
        !m_applyPreflightOutput) return false;
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe) return false;
    const bool hasPlan = m_applyPreflightPlan.available();
    const bool hasRollback = m_applyPreflightRollback.available();
    if (hasPlan != hasRollback) return false;
    if (!hasPlan) return finalizeApplyPreflight();

    String planLine;
    String rollbackLine;
    if (!readInspectionLine(m_applyPreflightPlan, planLine) ||
        !readInspectionLine(m_applyPreflightRollback, rollbackLine))
        return false;
    String planRemote;
    String logicalName;
    String planTarget;
    uint32_t stagedBytes = 0UL;
    String rollbackRemote;
    String rollbackTarget;
    bool previousPresent = false;
    uint32_t previousBytes = 0UL;
    uint32_t previousCrc = 0UL;
    if (!parseRestorePlanEntry(planLine, planRemote, stagedBytes,
                               logicalName, planTarget) ||
        !parseRollbackManifestEntry(rollbackLine, rollbackRemote,
                                    rollbackTarget, previousPresent,
                                    previousBytes, previousCrc) ||
        planRemote != rollbackRemote || planTarget != rollbackTarget)
        return false;
    String resolvedLogical;
    String resolvedTarget;
    if (!BackupExportWeb::resolveRestoreTarget(m_inspectionBatchId,
                                               planRemote,
                                               resolvedLogical,
                                               resolvedTarget) ||
        resolvedLogical != logicalName || resolvedTarget != planTarget ||
        m_applyPreflightFiles >= m_inspectionDataFiles ||
        stagedBytes > 1073741824UL - m_applyPreflightBytes)
        return false;

    m_applyPreflightRemoteName = planRemote;
    m_applyPreflightTargetPath = planTarget;
    m_applyPreflightStagedPath = String(StagingDirectory) + '/' + planRemote;
    m_applyPreflightRollbackPath = String(RollbackDirectory) + '/' + planRemote;
    m_applyPreflightPreviousPresent = previousPresent;
    m_applyPreflightStagedBytes = stagedBytes;
    m_applyPreflightPreviousBytes = previousBytes;
    m_applyPreflightPreviousCrc = previousCrc;
    m_applyPreflightCurrentCrc = 0UL;
    m_applyPreflightRollbackCrc = 0UL;
    m_applyPreflightStagedCrc = 0UL;

    File staged = m_storage.open(m_applyPreflightStagedPath, FILE_READ);
    const bool stagedValid = staged && !staged.isDirectory() &&
                             staged.size() == stagedBytes;
    if (staged) staged.close();
    if (!stagedValid) return false;

    if (!previousPresent)
    {
        if (m_storage.exists(m_applyPreflightTargetPath) ||
            m_storage.exists(m_applyPreflightRollbackPath)) return false;
        return openApplyPreflightInput(
            m_applyPreflightStagedPath, stagedBytes,
            static_cast<uint8_t>(ApplyPreflightStage::StagedFile));
    }
    File rollback = m_storage.open(m_applyPreflightRollbackPath, FILE_READ);
    const bool rollbackValid = rollback && !rollback.isDirectory() &&
                               rollback.size() == previousBytes;
    if (rollback) rollback.close();
    File current = m_storage.open(m_applyPreflightTargetPath, FILE_READ);
    const bool currentValid = current && !current.isDirectory() &&
                              current.size() == previousBytes;
    if (current) current.close();
    if (!rollbackValid || !currentValid) return false;
    return openApplyPreflightInput(
        m_applyPreflightTargetPath, previousBytes,
        static_cast<uint8_t>(ApplyPreflightStage::CurrentFile));
}

bool RemoteBackupWeb::openApplyPreflightInput(const String& path,
                                               uint32_t expectedBytes,
                                               uint8_t nextStage)
{
    if (m_applyPreflightInput) m_applyPreflightInput.close();
    m_applyPreflightInput = m_storage.open(path, FILE_READ);
    if (!m_applyPreflightInput || m_applyPreflightInput.isDirectory() ||
        m_applyPreflightInput.size() != expectedBytes)
    {
        if (m_applyPreflightInput) m_applyPreflightInput.close();
        return false;
    }
    m_applyPreflightInputBytes = 0UL;
    m_applyPreflightInputExpected = expectedBytes;
    m_applyPreflightInputCrc = 0xFFFFFFFFUL;
    m_applyPreflightStage = static_cast<ApplyPreflightStage>(nextStage);
    return true;
}

bool RemoteBackupWeb::continueApplyPreflightCrc()
{
    if (!m_applyPreflightInput) return false;
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    if (activity != BackupActivityCheck::Safe) return false;
    uint8_t buffer[1024];
    if (m_applyPreflightInput.available())
    {
        const int read = m_applyPreflightInput.read(buffer, sizeof(buffer));
        if (read <= 0) return false;
        m_applyPreflightInputCrc = updateCrc32(
            m_applyPreflightInputCrc, buffer, static_cast<size_t>(read));
        m_applyPreflightInputBytes += static_cast<uint32_t>(read);
        return m_applyPreflightInputBytes <= m_applyPreflightInputExpected;
    }
    m_applyPreflightInput.close();
    if (m_applyPreflightInputBytes != m_applyPreflightInputExpected)
        return false;
    const uint32_t crc = m_applyPreflightInputCrc ^ 0xFFFFFFFFUL;
    if (m_applyPreflightStage == ApplyPreflightStage::CurrentFile)
    {
        if (crc != m_applyPreflightPreviousCrc) return false;
        m_applyPreflightCurrentCrc = crc;
        return openApplyPreflightInput(
            m_applyPreflightRollbackPath, m_applyPreflightPreviousBytes,
            static_cast<uint8_t>(ApplyPreflightStage::RollbackFile));
    }
    if (m_applyPreflightStage == ApplyPreflightStage::RollbackFile)
    {
        if (crc != m_applyPreflightPreviousCrc ||
            crc != m_applyPreflightCurrentCrc) return false;
        m_applyPreflightRollbackCrc = crc;
        return openApplyPreflightInput(
            m_applyPreflightStagedPath, m_applyPreflightStagedBytes,
            static_cast<uint8_t>(ApplyPreflightStage::StagedFile));
    }
    if (m_applyPreflightStage != ApplyPreflightStage::StagedFile) return false;
    m_applyPreflightStagedCrc = crc;
    if (!appendApplyPreflightEntry()) return false;
    ++m_applyPreflightFiles;
    m_applyPreflightBytes += m_applyPreflightStagedBytes;
    m_applyPreflightInputBytes = 0UL;
    m_applyPreflightInputExpected = 0UL;
    m_applyPreflightStage = ApplyPreflightStage::Entries;
    return true;
}

bool RemoteBackupWeb::appendApplyPreflightEntry()
{
    if (!m_applyPreflightOutput) return false;
    String stagedCrc(m_applyPreflightStagedCrc, HEX);
    while (stagedCrc.length() < 8U) stagedCrc = String('0') + stagedCrc;
    String line = m_applyPreflightRemoteName;
    line += '\t'; line += m_applyPreflightTargetPath;
    line += '\t'; line += m_applyPreflightStagedBytes;
    line += '\t'; line += stagedCrc;
    line += m_applyPreflightPreviousPresent ? F("\tPRESENT\t")
                                            : F("\tMISSING\t");
    line += m_applyPreflightPreviousBytes;
    line += '\t';
    if (m_applyPreflightPreviousPresent)
    {
        String previousCrc(m_applyPreflightPreviousCrc, HEX);
        while (previousCrc.length() < 8U)
            previousCrc = String('0') + previousCrc;
        line += previousCrc;
    }
    else line += '-';
    line += '\n';
    const bool written = m_applyPreflightOutput.print(line) == line.length();
    m_applyPreflightOutput.flush();
    return written;
}

bool RemoteBackupWeb::finalizeApplyPreflight()
{
    if (m_applyPreflightPlan) m_applyPreflightPlan.close();
    if (m_applyPreflightRollback) m_applyPreflightRollback.close();
    if (!m_applyPreflightOutput ||
        m_applyPreflightFiles != m_inspectionDataFiles ||
        m_applyPreflightBytes != m_inspectionTotalBytes)
        return false;
    m_applyPreflightOutput.flush();
    m_applyPreflightOutput.close();
    if (!m_storage.rename(ApplyPreflightTempPath, ApplyPreflightPath))
        return false;
    File marker = m_storage.open(ApplyReadyMarkerPath, FILE_WRITE);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String content = F("COILMASTER_APPLY_READY_V1\nbatch_id=");
    content += m_inspectionBatchId;
    content += F("\nfiles="); content += m_applyPreflightFiles;
    content += F("\nstaged_bytes="); content += m_applyPreflightBytes;
    content += F("\nrestore_apply_enabled=0\nworking_data_changed=0\n");
    const bool written = marker.print(content) == content.length();
    marker.flush();
    marker.close();
    if (!written) return false;
    m_applyPreflightInputBytes = 0UL;
    m_applyPreflightInputExpected = 0UL;
    m_applyPreflightStage = ApplyPreflightStage::Complete;
    return true;
}

void RemoteBackupWeb::failApplyPreflight(const char* reason)
{
    if (m_applyPreflightPlan) m_applyPreflightPlan.close();
    if (m_applyPreflightRollback) m_applyPreflightRollback.close();
    if (m_applyPreflightOutput) m_applyPreflightOutput.close();
    if (m_applyPreflightInput) m_applyPreflightInput.close();
    m_storage.remove(ApplyPreflightTempPath);
    m_storage.remove(ApplyPreflightPath);
    m_storage.remove(ApplyReadyMarkerPath);
    m_applyPreflightError = reason != nullptr
                                ? reason : "apply_preflight_failed";
    m_applyPreflightStage = ApplyPreflightStage::Failed;
}

bool RemoteBackupWeb::applyPreflightActive() const
{
    return m_applyPreflightStage == ApplyPreflightStage::Entries ||
           m_applyPreflightStage == ApplyPreflightStage::CurrentFile ||
           m_applyPreflightStage == ApplyPreflightStage::RollbackFile ||
           m_applyPreflightStage == ApplyPreflightStage::StagedFile;
}

bool RemoteBackupWeb::validateApplyReadyMarker() const
{
    File marker = m_storage.open(ApplyReadyMarkerPath, FILE_READ);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(marker, line) &&
                 line == F("COILMASTER_APPLY_READY_V1");
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == m_inspectionBatchId;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line.startsWith("staged_bytes=") &&
                parseCanonicalUnsigned(line.substring(13), 0UL,
                                       1073741824UL, parsed) &&
                parsed == m_inspectionTotalBytes;
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line == F("restore_apply_enabled=0");
    if (valid)
        valid = readInspectionLine(marker, line) &&
                line == F("working_data_changed=0") && !marker.available();
    marker.close();
    return valid;
}

bool RemoteBackupWeb::processNextApplyEntry()
{
    if (!m_applyManifest || !m_applyJournal ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    if (!m_applyManifest.available()) return finalizeApply();

    const String entry = m_applyManifest.readStringUntil('\n');
    String remoteName;
    String targetPath;
    uint32_t stagedBytes = 0UL;
    uint32_t stagedCrc = 0UL;
    bool previousPresent = false;
    uint32_t previousBytes = 0UL;
    uint32_t previousCrc = 0UL;
    if (!parseApplyPreflightEntry(entry, remoteName, targetPath,
                                  stagedBytes, stagedCrc, previousPresent,
                                  previousBytes, previousCrc) ||
        m_applyFiles >= m_inspectionDataFiles ||
        stagedBytes > 1073741824UL - m_applyBytes)
        return false;
    String resolvedLogical;
    String resolvedTarget;
    if (!BackupExportWeb::resolveRestoreTarget(m_inspectionBatchId,
                                               remoteName,
                                               resolvedLogical,
                                               resolvedTarget) ||
        resolvedTarget != targetPath) return false;

    const String stagedPath = String(StagingDirectory) + '/' + remoteName;
    const String rollbackPath = String(RollbackDirectory) + '/' + remoteName;
    File staged = m_storage.open(stagedPath, FILE_READ);
    const bool stagedValid = staged && !staged.isDirectory() &&
                             staged.size() == stagedBytes;
    if (staged) staged.close();
    if (!stagedValid) return false;
    if (previousPresent)
    {
        File current = m_storage.open(targetPath, FILE_READ);
        const bool currentValid = current && !current.isDirectory() &&
                                  current.size() == previousBytes;
        if (current) current.close();
        File rollback = m_storage.open(rollbackPath, FILE_READ);
        const bool rollbackValid = rollback && !rollback.isDirectory() &&
                                   rollback.size() == previousBytes;
        if (rollback) rollback.close();
        if (!currentValid || !rollbackValid) return false;
    }
    else if (m_storage.exists(targetPath) || m_storage.exists(rollbackPath))
        return false;

    m_applyRemoteName = remoteName;
    m_applyTargetPath = targetPath;
    m_applyRollbackPath = rollbackPath;
    m_applyPreviousPresent = previousPresent;
    m_applyPreviousBytes = previousBytes;
    m_applyPreviousCrc = previousCrc;
    if (!appendApplyJournalEntry(entry)) return false;
    const String temporaryPath = targetPath + F(".cm-restore.part");
    return startApplyCopy(stagedPath, temporaryPath, stagedBytes, stagedCrc,
                          false);
}

bool RemoteBackupWeb::startApplyCopy(const String& sourcePath,
                                     const String& temporaryPath,
                                     uint32_t expectedBytes,
                                     uint32_t expectedCrc,
                                     bool rollbackCopy)
{
    if (m_applySource) m_applySource.close();
    if (m_applyDestination) m_applyDestination.close();
    if (m_storage.exists(temporaryPath)) return false;
    m_applySource = m_storage.open(sourcePath, FILE_READ);
    m_applyDestination = m_storage.open(temporaryPath, FILE_WRITE);
    if (!m_applySource || m_applySource.isDirectory() ||
        m_applySource.size() != expectedBytes ||
        !m_applyDestination || m_applyDestination.isDirectory())
    {
        if (m_applySource) m_applySource.close();
        if (m_applyDestination) m_applyDestination.close();
        m_storage.remove(temporaryPath);
        return false;
    }
    m_applyTemporaryPath = temporaryPath;
    m_applyExpectedBytes = expectedBytes;
    m_applyExpectedCrc = expectedCrc;
    m_applyCopiedBytes = 0UL;
    m_applyCopyCrc = 0xFFFFFFFFUL;
    m_applyVerifyBytes = 0UL;
    m_applyVerifyCrc = 0xFFFFFFFFUL;
    m_applyRollbackCopy = rollbackCopy;
    m_applyStage = rollbackCopy ? ApplyStage::RollbackCopying
                                : ApplyStage::Copying;
    return true;
}

bool RemoteBackupWeb::continueApplyCopy()
{
    if (!m_applySource || !m_applyDestination ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    uint8_t buffer[1024];
    if (m_applySource.available())
    {
        const size_t remaining = static_cast<size_t>(
            m_applyExpectedBytes - m_applyCopiedBytes);
        const size_t requested = remaining < sizeof(buffer) ? remaining
                                                            : sizeof(buffer);
        if (requested == 0U) return false;
        const int read = m_applySource.read(buffer, requested);
        if (read <= 0 ||
            m_applyDestination.write(buffer, static_cast<size_t>(read)) !=
                static_cast<size_t>(read)) return false;
        m_applyCopyCrc = updateCrc32(m_applyCopyCrc, buffer,
                                     static_cast<size_t>(read));
        m_applyCopiedBytes += static_cast<uint32_t>(read);
        return m_applyCopiedBytes <= m_applyExpectedBytes;
    }
    m_applySource.close();
    m_applyDestination.flush();
    m_applyDestination.close();
    if (m_applyCopiedBytes != m_applyExpectedBytes ||
        (m_applyCopyCrc ^ 0xFFFFFFFFUL) != m_applyExpectedCrc)
        return false;
    m_applySource = m_storage.open(m_applyTemporaryPath, FILE_READ);
    if (!m_applySource || m_applySource.isDirectory() ||
        m_applySource.size() != m_applyExpectedBytes)
    {
        if (m_applySource) m_applySource.close();
        return false;
    }
    m_applyVerifyBytes = 0UL;
    m_applyVerifyCrc = 0xFFFFFFFFUL;
    m_applyStage = m_applyRollbackCopy ? ApplyStage::RollbackVerifying
                                       : ApplyStage::Verifying;
    return true;
}

bool RemoteBackupWeb::continueApplyVerification()
{
    if (!m_applySource ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    uint8_t buffer[1024];
    if (m_applySource.available())
    {
        const int read = m_applySource.read(buffer, sizeof(buffer));
        if (read <= 0) return false;
        m_applyVerifyCrc = updateCrc32(m_applyVerifyCrc, buffer,
                                       static_cast<size_t>(read));
        m_applyVerifyBytes += static_cast<uint32_t>(read);
        return m_applyVerifyBytes <= m_applyExpectedBytes;
    }
    m_applySource.close();
    if (m_applyVerifyBytes != m_applyExpectedBytes ||
        (m_applyVerifyCrc ^ 0xFFFFFFFFUL) != m_applyExpectedCrc)
        return false;
    if (m_storage.exists(m_applyTargetPath) &&
        !m_storage.remove(m_applyTargetPath)) return false;
    if (!m_storage.rename(m_applyTemporaryPath, m_applyTargetPath)) return false;
    File installed = m_storage.open(m_applyTargetPath, FILE_READ);
    const bool installedValid = installed && !installed.isDirectory() &&
                                installed.size() == m_applyExpectedBytes;
    if (installed) installed.close();
    if (!installedValid) return false;
    m_applyTemporaryPath = String();
    m_applyCopiedBytes = 0UL;
    m_applyExpectedBytes = 0UL;
    if (m_applyRollbackCopy)
    {
        ++m_applyRestoredFiles;
        m_applyStage = ApplyStage::RollbackEntries;
    }
    else
    {
        ++m_applyFiles;
        m_applyBytes += m_applyVerifyBytes;
        m_applyStage = ApplyStage::Entries;
    }
    return true;
}

bool RemoteBackupWeb::appendApplyJournalEntry(const String& source)
{
    if (!m_applyJournal || source.length() == 0U || source.length() > 500U)
        return false;
    String line = source;
    if (line.endsWith("\r")) line.remove(line.length() - 1U);
    line += '\n';
    const bool written = m_applyJournal.print(line) == line.length();
    m_applyJournal.flush();
    return written;
}

bool RemoteBackupWeb::finalizeApply()
{
    if (m_applyManifest) m_applyManifest.close();
    if (!m_applyJournal || m_applyFiles != m_inspectionDataFiles ||
        m_applyBytes != m_inspectionTotalBytes) return false;
    m_applyJournal.flush();
    m_applyJournal.close();
    if (m_storage.exists(ApplyResultMarkerPath) &&
        !m_storage.remove(ApplyResultMarkerPath)) return false;
    File marker = m_storage.open(ApplyResultMarkerPath, FILE_WRITE);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String content = F("COILMASTER_APPLY_RESULT_V1\nbatch_id=");
    content += m_inspectionBatchId;
    content += F("\nstate=APPLIED\nfiles="); content += m_applyFiles;
    content += F("\nbytes="); content += m_applyBytes;
    content += F("\nreboot_required=1\nauto_resume=0\n");
    const bool written = marker.print(content) == content.length();
    marker.flush();
    marker.close();
    if (!written) return false;
    m_applyStage = ApplyStage::Complete;
    return true;
}

bool RemoteBackupWeb::beginApplyRollback(const char* reason)
{
    if (m_applyManifest) m_applyManifest.close();
    if (m_applyJournal) m_applyJournal.close();
    if (m_applySource) m_applySource.close();
    if (m_applyDestination) m_applyDestination.close();
    if (m_applyTemporaryPath.length() > 0U)
        m_storage.remove(m_applyTemporaryPath);
    m_applyRollbackReason = reason != nullptr
                                ? reason : "restore_apply_failed";
    m_applyJournal = m_storage.open(ApplyJournalPath, FILE_READ);
    if (!m_applyJournal || m_applyJournal.isDirectory())
    {
        if (m_applyJournal) m_applyJournal.close();
        return false;
    }
    String line;
    uint32_t parsed = 0UL;
    bool valid = readInspectionLine(m_applyJournal, line) &&
                 line == F("COILMASTER_APPLY_JOURNAL_V1");
    if (valid)
        valid = readInspectionLine(m_applyJournal, line) &&
                line.startsWith("batch_id=") &&
                parseCanonicalUnsigned(line.substring(9), 1UL,
                                       0xFFFFFFFFUL, parsed) &&
                parsed == m_inspectionBatchId;
    if (valid)
        valid = readInspectionLine(m_applyJournal, line) &&
                line.startsWith("files=") &&
                parseCanonicalUnsigned(line.substring(6), 1UL, 4096UL,
                                       parsed) &&
                parsed == m_inspectionDataFiles;
    if (valid)
        valid = readInspectionLine(m_applyJournal, line) &&
                line == F("state=RUNNING") &&
                readInspectionLine(m_applyJournal, line) &&
                line.length() == 0U;
    if (!valid)
    {
        m_applyJournal.close();
        return false;
    }
    m_applyRestoredFiles = 0UL;
    m_applyStage = ApplyStage::RollbackEntries;
    return true;
}

bool RemoteBackupWeb::processNextApplyRollbackEntry()
{
    if (!m_applyJournal ||
        BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
        return false;
    if (!m_applyJournal.available()) return finalizeApplyRollback();
    const String entry = m_applyJournal.readStringUntil('\n');
    String remoteName;
    String targetPath;
    uint32_t stagedBytes = 0UL;
    uint32_t stagedCrc = 0UL;
    bool previousPresent = false;
    uint32_t previousBytes = 0UL;
    uint32_t previousCrc = 0UL;
    if (!parseApplyPreflightEntry(entry, remoteName, targetPath,
                                  stagedBytes, stagedCrc, previousPresent,
                                  previousBytes, previousCrc)) return false;
    String resolvedLogical;
    String resolvedTarget;
    if (!BackupExportWeb::resolveRestoreTarget(m_inspectionBatchId,
                                               remoteName,
                                               resolvedLogical,
                                               resolvedTarget) ||
        resolvedTarget != targetPath) return false;
    m_applyRemoteName = remoteName;
    m_applyTargetPath = targetPath;
    m_applyRollbackPath = String(RollbackDirectory) + '/' + remoteName;
    m_applyPreviousPresent = previousPresent;
    m_applyPreviousBytes = previousBytes;
    m_applyPreviousCrc = previousCrc;
    if (!previousPresent)
    {
        if (m_storage.exists(targetPath) && !m_storage.remove(targetPath))
            return false;
        ++m_applyRestoredFiles;
        return true;
    }
    File rollback = m_storage.open(m_applyRollbackPath, FILE_READ);
    const bool rollbackValid = rollback && !rollback.isDirectory() &&
                               rollback.size() == previousBytes;
    if (rollback) rollback.close();
    if (!rollbackValid) return false;
    const String temporaryPath = targetPath + F(".cm-restore.part");
    return startApplyCopy(m_applyRollbackPath, temporaryPath,
                          previousBytes, previousCrc, true);
}

bool RemoteBackupWeb::finalizeApplyRollback()
{
    if (m_applyJournal) m_applyJournal.close();
    if (m_storage.exists(ApplyResultMarkerPath) &&
        !m_storage.remove(ApplyResultMarkerPath)) return false;
    File marker = m_storage.open(ApplyResultMarkerPath, FILE_WRITE);
    if (!marker || marker.isDirectory())
    {
        if (marker) marker.close();
        return false;
    }
    String content = F("COILMASTER_APPLY_RESULT_V1\nbatch_id=");
    content += m_inspectionBatchId;
    content += F("\nstate=ROLLED_BACK\nfiles_restored=");
    content += m_applyRestoredFiles;
    content += F("\nreason="); content += m_applyRollbackReason;
    content += F("\nreboot_required=0\nauto_resume=0\n");
    const bool written = marker.print(content) == content.length();
    marker.flush();
    marker.close();
    if (!written) return false;
    m_applyStage = ApplyStage::RolledBack;
    return true;
}

void RemoteBackupWeb::failApply(const char* reason)
{
    if (m_applyManifest) m_applyManifest.close();
    if (m_applyJournal) m_applyJournal.close();
    if (m_applySource) m_applySource.close();
    if (m_applyDestination) m_applyDestination.close();
    if (m_applyTemporaryPath.length() > 0U)
        m_storage.remove(m_applyTemporaryPath);
    m_applyError = reason != nullptr ? reason : "restore_apply_failed";
    m_applyStage = ApplyStage::Failed;
}

bool RemoteBackupWeb::applyActive() const
{
    return m_applyStage == ApplyStage::Entries ||
           m_applyStage == ApplyStage::Copying ||
           m_applyStage == ApplyStage::Verifying ||
           m_applyStage == ApplyStage::RollbackEntries ||
           m_applyStage == ApplyStage::RollbackCopying ||
           m_applyStage == ApplyStage::RollbackVerifying;
}

uint32_t RemoteBackupWeb::dateKey(const RtcDateTime& value)
{
    return static_cast<uint32_t>(value.year) * 10000UL +
           static_cast<uint32_t>(value.month) * 100UL + value.day;
}

void RemoteBackupWeb::updateSchedule(uint32_t nowMs)
{
    if (m_lastScheduleCheckMs != 0UL &&
        static_cast<uint32_t>(nowMs - m_lastScheduleCheckMs) < 30000UL)
        return;
    m_lastScheduleCheckMs = nowMs;

    RemoteBackupSettings settings;
    bool configured = false;
    if (!m_settingsStore.load(settings, configured) || !configured)
    {
        m_scheduleState = F("SETTINGS_UNAVAILABLE");
        return;
    }
    if (!settings.enabled || !settings.scheduleEnabled)
    {
        m_scheduleState = F("DISABLED");
        return;
    }

    RtcDateTime now;
    if (!m_rtcClock.read(now))
    {
        m_scheduleState = F("RTC_INVALID");
        return;
    }
    const uint32_t today = dateKey(now);
    if (settings.lastScheduledDate == today)
    {
        m_scheduleState = F("COMPLETED_TODAY");
        return;
    }
    if (m_scheduleAttemptDate == today) return;
    if (now.hour < settings.scheduleHour ||
        (now.hour == settings.scheduleHour &&
         now.minute < settings.scheduleMinute))
    {
        m_scheduleState = F("WAITING_TIME");
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        m_scheduleState = F("WAITING_STA");
        return;
    }
    if (BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe)
    {
        m_scheduleState = F("WAITING_IDLE");
        return;
    }

    m_scheduleAttemptDate = today;
    m_scheduledBatchDate = today;
    uint16_t statusCode = 500U;
    const char* error = "scheduled_backup_start_failed";
    if (!startFullBatch(true, statusCode, error))
    {
        (void)statusCode;
        m_scheduleState = error != nullptr ? error : "SCHEDULED_START_FAILED";
        m_batchScheduled = false;
        m_scheduledBatchDate = 0UL;
        return;
    }
    m_scheduleState = F("RUNNING");
}

void RemoteBackupWeb::completeScheduledBatch()
{
    if (!m_batchScheduled) return;
    m_batchSettings.lastScheduledDate = m_scheduledBatchDate;
    if (m_scheduledBatchDate > 0UL && m_settingsStore.save(m_batchSettings))
        m_scheduleState = F("COMPLETED_TODAY");
    else
        m_scheduleState = F("STATE_WRITE_FAILED");
    m_batchScheduled = false;
    m_scheduledBatchDate = 0UL;
}

bool RemoteBackupWeb::allocateBatchId(uint32_t& batchId)
{
    batchId = 0UL;
    auto readSequence = [this](const char* path, uint32_t& value) -> bool
    {
        value = 0UL;
        if (!m_storage.exists(path)) return false;
        File file = m_storage.open(path, FILE_READ);
        if (!file || file.isDirectory())
        { if (file) file.close(); return false; }
        const String line = file.readStringUntil('\n');
        const String extra = file.readStringUntil('\n');
        file.close();
        return extra.length() == 0U &&
               parseCanonicalUnsigned(line, 0UL, 0xFFFFFFFEUL, value);
    };
    uint32_t mainValue = 0UL, tempValue = 0UL, backupValue = 0UL;
    bool mainValid = readSequence(BatchSequencePath, mainValue);
    const bool tempExists = m_storage.exists(BatchSequenceTempPath);
    const bool backupExists = m_storage.exists(BatchSequenceBackupPath);
    if (tempExists || backupExists)
    {
        const bool tempValid = readSequence(BatchSequenceTempPath, tempValue);
        const bool backupValid = readSequence(BatchSequenceBackupPath, backupValue);
        if (mainValid)
        {
            if (tempExists && !m_storage.remove(BatchSequenceTempPath)) return false;
            if (backupExists && !m_storage.remove(BatchSequenceBackupPath)) return false;
        }
        else if (tempValid)
        {
            if (m_storage.exists(BatchSequencePath) &&
                !m_storage.remove(BatchSequencePath)) return false;
            if (!m_storage.rename(BatchSequenceTempPath, BatchSequencePath)) return false;
            if (backupExists && !m_storage.remove(BatchSequenceBackupPath)) return false;
            mainValue = tempValue;
            mainValid = true;
        }
        else if (backupValid)
        {
            if (m_storage.exists(BatchSequencePath) &&
                !m_storage.remove(BatchSequencePath)) return false;
            if (tempExists && !m_storage.remove(BatchSequenceTempPath)) return false;
            if (!m_storage.rename(BatchSequenceBackupPath, BatchSequencePath)) return false;
            mainValue = backupValue;
            mainValid = true;
        }
        else return false;
    }
    uint32_t current = 0UL;
    if (m_storage.exists(BatchSequencePath))
    {
        if (!mainValid) return false;
        current = mainValue;
    }
    const uint32_t next = current + 1UL;
    File temp = m_storage.open(BatchSequenceTempPath, FILE_WRITE);
    if (!temp) return false;
    String value = String(next) + '\n';
    const bool written = temp.print(value) == value.length();
    temp.flush(); temp.close();
    if (!written) { m_storage.remove(BatchSequenceTempPath); return false; }
    if (m_storage.exists(BatchSequenceBackupPath) &&
        !m_storage.remove(BatchSequenceBackupPath))
    { m_storage.remove(BatchSequenceTempPath); return false; }
    if (m_storage.exists(BatchSequencePath) &&
        !m_storage.rename(BatchSequencePath, BatchSequenceBackupPath))
    { m_storage.remove(BatchSequenceTempPath); return false; }
    if (!m_storage.rename(BatchSequenceTempPath, BatchSequencePath))
    {
        if (!m_storage.exists(BatchSequencePath) &&
            m_storage.exists(BatchSequenceBackupPath))
            m_storage.rename(BatchSequenceBackupPath, BatchSequencePath);
        return false;
    }
    if (m_storage.exists(BatchSequenceBackupPath) &&
        !m_storage.remove(BatchSequenceBackupPath)) return false;
    batchId = next;
    return true;
}

bool RemoteBackupWeb::startNextBatchFile()
{
    const String prefix = String(F("cm-b")) + m_batchId + '-';
    String logical, path, name;
    if (m_batchStage == BatchStage::MainFiles)
    {
        while (m_batchMainIndex < BackupExportWeb::exportFileCount())
        {
            if (!BackupExportWeb::resolveExportFileAt(m_batchMainIndex++,
                                                      logical, path, name))
                return false;
            if (!m_storage.exists(path)) continue;
            return startTrackedBatchFile(logical, path,
                                         prefix + F("main-") + name);
        }
        m_batchStage = BatchStage::SessionFiles;
    }
    while (m_batchStage == BatchStage::SessionFiles)
    {
        if (m_batchSessionId == 0UL)
        {
            bool found = false;
            if (!BackupExportWeb::nextSessionId(m_storage,
                                                m_batchAfterSessionId,
                                                m_batchSessionId,
                                                found)) return false;
            if (!found)
            {
                m_batchStage = BatchStage::Manifest;
                break;
            }
            m_batchKindIndex = 0U;
        }
        while (m_batchKindIndex < 3U)
        {
            bool exists = false;
            if (!BackupExportWeb::resolveSessionFile(m_storage,
                                                     m_batchSessionId,
                                                     m_batchKindIndex++,
                                                     logical, path, name,
                                                     exists)) return false;
            if (!exists) continue;
            return startTrackedBatchFile(logical, path,
                                         prefix + F("session-") + name);
        }
        m_batchAfterSessionId = m_batchSessionId;
        m_batchSessionId = 0UL;
    }
    if (m_batchStage == BatchStage::Manifest)
    {
        if (!createRemoteBatchManifest()) return false;
        return startTrackedBatchFile(F("batch-manifest"),
                                     RemoteBatchManifestPath,
                                     prefix + F("MANIFEST.txt"));
    }
    if (m_batchStage == BatchStage::Marker)
    {
        String stabilityReason;
        if (!BackupExportWeb::snapshotStable(m_storage, stabilityReason) ||
            !createCompletionMarker()) return false;
        return startTrackedBatchFile(F("batch-complete"), BatchMarkerPath,
                                     prefix + F("COMPLETE.txt"));
    }
    return false;
}

bool RemoteBackupWeb::startTrackedBatchFile(const String& logicalName,
                                             const String& localPath,
                                             const String& remoteName)
{
    File source = m_storage.open(localPath, FILE_READ);
    if (!source || source.isDirectory())
    {
        if (source) source.close();
        return false;
    }
    const uint32_t expectedBytes = static_cast<uint32_t>(source.size());
    source.close();
    return appendBatchManifestEntry(remoteName, expectedBytes) &&
           m_transfer.start(m_batchSettings, logicalName, localPath,
                            remoteName);
}

bool RemoteBackupWeb::createCompletionMarker()
{
    if (m_storage.exists(BatchMarkerPath) &&
        !m_storage.remove(BatchMarkerPath)) return false;
    File marker = m_storage.open(BatchMarkerPath, FILE_WRITE);
    if (!marker) return false;
    String content = F("COILMASTER_BACKUP_COMPLETE\nbatch_id=");
    content += m_batchId;
    content += F("\nfiles_before_marker=");
    content += m_batchFilesCompleted;
    content += '\n';
    const bool written = marker.print(content) == content.length();
    marker.flush(); marker.close();
    return written;
}

bool RemoteBackupWeb::createRemoteBatchManifest()
{
    if (m_storage.exists(RemoteBatchManifestPath) &&
        !m_storage.remove(RemoteBatchManifestPath)) return false;

    File source = m_storage.open(batchManifestPath(m_batchId, true), FILE_READ);
    if (!source || source.isDirectory())
    {
        if (source) source.close();
        return false;
    }
    File output = m_storage.open(RemoteBatchManifestPath, FILE_WRITE);
    if (!output || output.isDirectory())
    {
        if (output) output.close();
        source.close();
        return false;
    }

    String header = F("COILMASTER_BACKUP_MANIFEST_V2\nbatch_id=");
    header += m_batchId;
    header += F("\ndata_files=");
    header += m_batchFilesCompleted;
    header += '\n';
    bool valid = output.print(header) == header.length();
    const String prefix = String(F("cm-b")) + m_batchId + '-';
    const String manifestName = prefix + F("MANIFEST.txt");
    const String markerName = prefix + F("COMPLETE.txt");
    uint32_t copied = 0UL;
    while (valid && source.available())
    {
        const String entry = source.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_batchId, remoteName,
                                       hasExpectedBytes, expectedBytes) ||
            !hasExpectedBytes || remoteName == manifestName ||
            remoteName == markerName)
        {
            valid = false;
            break;
        }
        String line = remoteName + '\t';
        line += expectedBytes;
        line += '\n';
        valid = output.print(line) == line.length();
        ++copied;
    }
    valid = valid && copied == m_batchFilesCompleted;
    output.flush();
    output.close();
    source.close();
    if (!valid) m_storage.remove(RemoteBatchManifestPath);
    return valid;
}

String RemoteBackupWeb::batchManifestPath(uint32_t batchId,
                                          bool temporary) const
{
    String path = String(BatchManifestDirectory) + '/';
    path += batchId;
    path += temporary ? F(".tmp") : F(".lst");
    return path;
}

bool RemoteBackupWeb::beginBatchManifest()
{
    if (!m_storage.exists(BatchManifestDirectory) &&
        !m_storage.mkdir(BatchManifestDirectory)) return false;
    File directory = m_storage.open(BatchManifestDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }
    directory.close();

    const String temporaryPath = batchManifestPath(m_batchId, true);
    const String finalPath = batchManifestPath(m_batchId, false);
    if ((m_storage.exists(temporaryPath) &&
         !m_storage.remove(temporaryPath)) ||
        m_storage.exists(finalPath)) return false;
    File manifest = m_storage.open(temporaryPath, FILE_WRITE);
    if (!manifest || manifest.isDirectory())
    {
        if (manifest) manifest.close();
        return false;
    }
    manifest.close();
    return true;
}

bool RemoteBackupWeb::appendBatchManifestEntry(const String& remoteName,
                                                uint32_t expectedBytes)
{
    const String prefix = String(F("cm-b")) + m_batchId + '-';
    if (!remoteName.startsWith(prefix) || remoteName.length() > 180U ||
        remoteName.indexOf('/') >= 0 || remoteName.indexOf("..") >= 0 ||
        remoteName.indexOf('\t') >= 0 || remoteName.indexOf('\r') >= 0 ||
        remoteName.indexOf('\n') >= 0)
        return false;
    File manifest = m_storage.open(batchManifestPath(m_batchId, true),
                                   FILE_APPEND);
    if (!manifest || manifest.isDirectory())
    {
        if (manifest) manifest.close();
        return false;
    }
    String line = remoteName + '\t';
    line += expectedBytes;
    line += '\n';
    const bool written = manifest.print(line) == line.length();
    manifest.flush();
    manifest.close();
    return written;
}

bool RemoteBackupWeb::finalizeBatchManifest()
{
    const String temporaryPath = batchManifestPath(m_batchId, true);
    const String finalPath = batchManifestPath(m_batchId, false);
    return m_storage.exists(temporaryPath) &&
           !m_storage.exists(finalPath) &&
           m_storage.rename(temporaryPath, finalPath);
}

bool RemoteBackupWeb::selectOldestManagedBatch(
    uint32_t& batchId,
    uint16_t& manifestCount) const
{
    batchId = 0UL;
    manifestCount = 0U;
    if (!m_storage.exists(BatchManifestDirectory)) return true;
    File directory = m_storage.open(BatchManifestDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }
    File entry = directory.openNextFile();
    while (entry)
    {
        if (!entry.isDirectory())
        {
            String name = entry.name();
            const int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(static_cast<unsigned>(slash + 1));
            if (name.endsWith(F(".lst")))
            {
                const String number = name.substring(0U, name.length() - 4U);
                uint32_t parsed = 0UL;
                if (!parseCanonicalUnsigned(number, 1UL, 0xFFFFFFFFUL,
                                            parsed) ||
                    manifestCount == 0xFFFFU)
                {
                    entry.close();
                    directory.close();
                    return false;
                }
                ++manifestCount;
                if (batchId == 0UL || parsed < batchId) batchId = parsed;
            }
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return true;
}

bool RemoteBackupWeb::startRetention()
{
    uint32_t incompleteBatchId = 0UL;
    if (!selectOldestIncompleteBatch(incompleteBatchId)) return false;
    if (incompleteBatchId > 0UL)
        return startIncompleteCleanup(incompleteBatchId);

    uint32_t oldestBatchId = 0UL;
    uint16_t manifestCount = 0U;
    if (!selectOldestManagedBatch(oldestBatchId, manifestCount)) return false;
    if (manifestCount <= m_batchSettings.retentionCount)
    {
        m_transfer.finishDeleteSession();
        m_batchStage = BatchStage::Complete;
        completeScheduledBatch();
        return true;
    }
    return startRetentionBatch(oldestBatchId);
}

bool RemoteBackupWeb::selectOldestIncompleteBatch(uint32_t& batchId) const
{
    batchId = 0UL;
    if (!m_storage.exists(BatchManifestDirectory)) return true;
    File directory = m_storage.open(BatchManifestDirectory, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }
    File entry = directory.openNextFile();
    while (entry)
    {
        if (!entry.isDirectory())
        {
            String name = entry.name();
            const int slash = name.lastIndexOf('/');
            if (slash >= 0)
                name = name.substring(static_cast<unsigned>(slash + 1));
            if (name.endsWith(F(".tmp")))
            {
                const String number = name.substring(0U, name.length() - 4U);
                uint32_t parsed = 0UL;
                if (!parseCanonicalUnsigned(number, 1UL, 0xFFFFFFFFUL,
                                            parsed))
                {
                    entry.close();
                    directory.close();
                    return false;
                }
                if (batchId == 0UL || parsed < batchId) batchId = parsed;
            }
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return true;
}

bool RemoteBackupWeb::startIncompleteCleanup(uint32_t batchId)
{
    if (batchId == 0UL || m_retentionManifest) return false;
    m_retentionBatchId = batchId;
    m_retentionMarkerDeleted = false;
    m_retentionIncomplete = true;
    m_retentionDeletingPart = false;
    m_retentionPendingName = String(F("cm-b")) + batchId +
                             F("-COMPLETE.txt");
    m_batchStage = BatchStage::Retention;
    return m_transfer.startDelete(m_batchSettings,
                                  F("incomplete-marker"),
                                  m_retentionPendingName);
}

bool RemoteBackupWeb::startNextIncompleteDelete()
{
    if (m_retentionPendingName.length() > 0U &&
        !m_retentionDeletingPart)
    {
        m_retentionDeletingPart = true;
        return m_transfer.startDelete(m_batchSettings,
                                      F("incomplete-part"),
                                      m_retentionPendingName + F(".part"));
    }

    m_retentionDeletingPart = false;
    m_retentionPendingName = String();
    if (!m_retentionManifest)
    {
        m_retentionManifest = m_storage.open(
            batchManifestPath(m_retentionBatchId, true), FILE_READ);
        if (!m_retentionManifest || m_retentionManifest.isDirectory())
        {
            if (m_retentionManifest) m_retentionManifest.close();
            return false;
        }
    }

    const String prefix = String(F("cm-b")) + m_retentionBatchId + '-';
    const String markerName = prefix + F("COMPLETE.txt");
    while (m_retentionManifest.available())
    {
        const String entry = m_retentionManifest.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_retentionBatchId, remoteName,
                                       hasExpectedBytes, expectedBytes))
            return false;
        (void)hasExpectedBytes;
        (void)expectedBytes;
        if (remoteName == markerName) continue;
        m_retentionPendingName = remoteName;
        return m_transfer.startDelete(m_batchSettings,
                                      F("incomplete-file"), remoteName);
    }

    m_retentionManifest.close();
    const String manifestPath = batchManifestPath(m_retentionBatchId, true);
    if (!m_storage.remove(manifestPath)) return false;
    ++m_incompleteBatchesDeleted;
    m_retentionBatchId = 0UL;
    m_retentionIncomplete = false;
    return startRetention();
}

bool RemoteBackupWeb::startRetentionBatch(uint32_t batchId)
{
    if (batchId == 0UL || m_retentionManifest) return false;
    m_retentionBatchId = batchId;
    m_retentionMarkerDeleted = false;
    m_batchStage = BatchStage::Retention;
    const String markerName = String(F("cm-b")) + batchId +
                              F("-COMPLETE.txt");
    return m_transfer.startDelete(m_batchSettings, F("retention-marker"),
                                  markerName);
}

bool RemoteBackupWeb::startNextRetentionFile()
{
    if (!m_retentionManifest)
    {
        m_retentionManifest = m_storage.open(
            batchManifestPath(m_retentionBatchId, false), FILE_READ);
        if (!m_retentionManifest || m_retentionManifest.isDirectory())
        {
            if (m_retentionManifest) m_retentionManifest.close();
            return false;
        }
    }

    const String prefix = String(F("cm-b")) + m_retentionBatchId + '-';
    const String markerName = prefix + F("COMPLETE.txt");
    while (m_retentionManifest.available())
    {
        const String entry = m_retentionManifest.readStringUntil('\n');
        String remoteName;
        bool hasExpectedBytes = false;
        uint32_t expectedBytes = 0UL;
        if (!parseManagedManifestEntry(entry, m_retentionBatchId, remoteName,
                                       hasExpectedBytes, expectedBytes))
            return false;
        (void)hasExpectedBytes;
        (void)expectedBytes;
        if (remoteName == markerName) continue;
        return m_transfer.startDelete(m_batchSettings, F("retention-file"),
                                      remoteName);
    }

    m_retentionManifest.close();
    const String manifestPath = batchManifestPath(m_retentionBatchId, false);
    if (!m_storage.remove(manifestPath)) return false;
    m_retentionBatchId = 0UL;
    return startRetention();
}

void RemoteBackupWeb::failRetention(const char* reason)
{
    if (m_retentionManifest) m_retentionManifest.close();
    m_transfer.finishDeleteSession();
    m_retentionSucceeded = false;
    m_retentionIncomplete = false;
    m_retentionDeletingPart = false;
    m_retentionPendingName = String();
    m_retentionError = reason != nullptr ? reason : "retention_failed";
    m_batchStage = BatchStage::Complete;
    completeScheduledBatch();
}

void RemoteBackupWeb::failBatch(const char* reason)
{
    m_batchError = reason != nullptr ? reason : "backup_batch_failed";
    m_storage.remove(BatchMarkerPath);
    m_storage.remove(RemoteBatchManifestPath);
    m_batchStage = BatchStage::Failed;
    if (m_batchScheduled) m_scheduleState = F("FAILED");
    m_batchScheduled = false;
    m_scheduledBatchDate = 0UL;
}
}
