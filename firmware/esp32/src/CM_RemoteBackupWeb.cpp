#include "CM_RemoteBackupWeb.h"

#include <WiFi.h>

#include "CM_BackupActivityGuard.h"
#include "CM_BackupExportWeb.h"

namespace CM
{
namespace
{
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
}

void RemoteBackupWeb::update(uint32_t nowMs)
{
    m_transfer.update(nowMs);
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
    if (m_transfer.active())
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
}
