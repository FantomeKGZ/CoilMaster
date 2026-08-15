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
constexpr const char* BatchManifestDirectory =
    "/data/settings/remote-backup-batches";

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
                                 RemoteBackupSettingsStore& settingsStore)
    : m_server(server),
      m_storage(storage),
      m_settingsStore(settingsStore),
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
}

void RemoteBackupWeb::update(uint32_t nowMs)
{
    m_transfer.update(nowMs);
    if (m_batchStage != BatchStage::MainFiles &&
        m_batchStage != BatchStage::SessionFiles &&
        m_batchStage != BatchStage::Marker &&
        m_batchStage != BatchStage::Retention) return;
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
    if (!appendBatchManifestName(m_transfer.remoteName()))
    {
        if (m_batchStage == BatchStage::Marker)
        {
            m_storage.remove(BatchMarkerPath);
            m_storage.remove(batchManifestPath(m_batchId, true));
            failRetention("batch_manifest_write_failed");
        }
        else
            failBatch("batch_manifest_write_failed");
        return;
    }
    ++m_batchFilesCompleted;
    if (m_batchStage == BatchStage::Marker)
    {
        m_storage.remove(BatchMarkerPath);
        if (!finalizeBatchManifest())
        {
            m_storage.remove(batchManifestPath(m_batchId, true));
            failRetention("batch_manifest_finalize_failed");
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
    response.reserve(340U);
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
    response += F(",\"transport\":\"FTP\",\"credentials_exposed\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void RemoteBackupWeb::handleSetConfiguration()
{
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
    m_server.send(200, "application/json; charset=utf-8",
                  "{\"saved\":true,\"credentials_exposed\":false}");
}

void RemoteBackupWeb::handleTestConnection()
{
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
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention)
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
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention)
    {
        m_server.send(409, "application/json",
                      "{\"error\":\"remote_backup_busy\"}");
        return;
    }
    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    String stabilityReason;
    if (activity != BackupActivityCheck::Safe ||
        !BackupExportWeb::snapshotStable(m_storage, stabilityReason))
    {
        m_server.send(activity == BackupActivityCheck::Unavailable ? 503 : 409,
                      "application/json",
                      "{\"error\":\"stable_snapshot_required\"}");
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
    if (!allocateBatchId(m_batchId))
    {
        m_server.send(500, "application/json",
                      "{\"error\":\"backup_batch_id_failed\"}");
        return;
    }
    m_batchMainIndex = 0U;
    m_batchAfterSessionId = 0UL;
    m_batchSessionId = 0UL;
    m_batchKindIndex = 0U;
    m_batchFilesCompleted = 0UL;
    m_batchError = String();
    m_retentionFilesDeleted = 0UL;
    m_retentionSucceeded = true;
    m_retentionOnly = false;
    m_retentionError = String();
    if (!beginBatchManifest())
    {
        failBatch("batch_manifest_create_failed");
        m_server.send(500, "application/json",
                      "{\"error\":\"backup_batch_manifest_failed\"}");
        return;
    }
    m_batchStage = BatchStage::MainFiles;
    if (!startNextBatchFile())
    {
        failBatch("batch_first_file_failed");
        m_server.send(500, "application/json",
                      "{\"error\":\"backup_batch_start_failed\"}");
        return;
    }
    String response = F("{\"accepted\":true,\"batch_id\":");
    response += m_batchId; response += '}';
    m_server.send(202, "application/json", response);
}

void RemoteBackupWeb::handleStartRetention()
{
    if (m_transfer.active() || m_batchStage == BatchStage::MainFiles ||
        m_batchStage == BatchStage::SessionFiles ||
        m_batchStage == BatchStage::Marker ||
        m_batchStage == BatchStage::Retention)
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
                 m_batchStage == BatchStage::Marker ||
                 m_batchStage == BatchStage::Retention) ? F("true") : F("false");
    response += F(",\"batch_id\":");
    if (m_batchId > 0UL) response += m_batchId; else response += F("null");
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
    response += F(",\"retention_succeeded\":");
    response += m_retentionSucceeded ? F("true") : F("false");
    response += F(",\"retention_error\":");
    if (m_retentionError.length() > 0U)
    { response += '"'; response += m_retentionError; response += '"'; }
    else response += F("null");
    response += '}';
    m_server.send(200, "application/json; charset=utf-8", response);
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
            return m_transfer.start(m_batchSettings, logical, path,
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
                m_batchStage = BatchStage::Marker;
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
            return m_transfer.start(m_batchSettings, logical, path,
                                    prefix + F("session-") + name);
        }
        m_batchAfterSessionId = m_batchSessionId;
        m_batchSessionId = 0UL;
    }
    if (m_batchStage == BatchStage::Marker)
    {
        String stabilityReason;
        if (!BackupExportWeb::snapshotStable(m_storage, stabilityReason) ||
            !createCompletionMarker()) return false;
        return m_transfer.start(m_batchSettings, F("batch-complete"),
                                BatchMarkerPath,
                                prefix + F("COMPLETE.txt"));
    }
    return false;
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

bool RemoteBackupWeb::appendBatchManifestName(const String& remoteName)
{
    const String prefix = String(F("cm-b")) + m_batchId + '-';
    if (!remoteName.startsWith(prefix) || remoteName.length() > 180U ||
        remoteName.indexOf('/') >= 0 || remoteName.indexOf("..") >= 0 ||
        remoteName.indexOf('\r') >= 0 || remoteName.indexOf('\n') >= 0)
        return false;
    File manifest = m_storage.open(batchManifestPath(m_batchId, true),
                                   FILE_APPEND);
    if (!manifest || manifest.isDirectory())
    {
        if (manifest) manifest.close();
        return false;
    }
    const String line = remoteName + '\n';
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
    uint32_t oldestBatchId = 0UL;
    uint16_t manifestCount = 0U;
    if (!selectOldestManagedBatch(oldestBatchId, manifestCount)) return false;
    if (manifestCount <= m_batchSettings.retentionCount)
    {
        m_transfer.finishDeleteSession();
        m_batchStage = BatchStage::Complete;
        return true;
    }
    return startRetentionBatch(oldestBatchId);
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
        String remoteName = m_retentionManifest.readStringUntil('\n');
        if (remoteName.endsWith("\r"))
            remoteName.remove(remoteName.length() - 1U);
        if (remoteName.length() == 0U || remoteName == markerName) continue;
        if (!remoteName.startsWith(prefix) || remoteName.length() > 180U ||
            remoteName.indexOf('/') >= 0 || remoteName.indexOf("..") >= 0 ||
            remoteName.indexOf('\r') >= 0 || remoteName.indexOf('\n') >= 0)
            return false;
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
    m_retentionError = reason != nullptr ? reason : "retention_failed";
    m_batchStage = BatchStage::Complete;
}

void RemoteBackupWeb::failBatch(const char* reason)
{
    m_batchError = reason != nullptr ? reason : "backup_batch_failed";
    m_storage.remove(BatchMarkerPath);
    if (m_batchId > 0UL)
        m_storage.remove(batchManifestPath(m_batchId, true));
    m_batchStage = BatchStage::Failed;
}
}
