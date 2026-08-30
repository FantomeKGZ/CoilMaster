#include "CM_WebRecoveryFtpServer.h"

namespace CM
{
namespace
{
constexpr char FtpUsername[] = "CoilMaster";
constexpr char FtpPassword[] = "CoilMaster123";
constexpr char WebRoot[] = "/web";

String jsonEscaped(const String& value)
{
    String result;
    result.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '"' || ch == '\\') result += '\\';
        if (static_cast<uint8_t>(ch) >= 0x20U) result += ch;
    }
    return result;
}

String baseName(const String& path)
{
    const int slash = path.lastIndexOf('/');
    return slash >= 0 ? path.substring(static_cast<unsigned int>(slash + 1)) : path;
}
}

WebRecoveryFtpServer::WebRecoveryFtpServer(WebServer& webServer,
                                           fs::FS& storage)
    : m_webServer(webServer),
      m_storage(storage),
      m_controlServer(ControlPort),
      m_dataServer(PassivePort),
      m_activityProbe(nullptr),
      m_transferState(TransferState::None),
      m_currentDirectory("/"),
      m_lastResult("not_started"),
      m_lastClientActivityMs(0UL),
      m_dataDeadlineMs(0UL),
      m_running(false),
      m_automaticRecovery(false),
      m_userAccepted(false),
      m_authenticated(false),
      m_passiveOpen(false),
      m_listNamesOnly(false)
{
}

void WebRecoveryFtpServer::begin(bool automaticRecovery)
{
    m_webServer.on("/api/ftp/status", HTTP_GET, [this]() { handleStatus(); });
    m_webServer.on("/api/ftp/start", HTTP_POST, [this]() { handleStart(); });
    m_webServer.on("/api/ftp/stop", HTTP_POST, [this]() { handleStop(); });
    if (automaticRecovery || !webRootUsable()) start(true);
}

void WebRecoveryFtpServer::setActivityProbe(BackupActivityGuard::RuntimeProbe probe)
{
    m_activityProbe = probe;
}

bool WebRecoveryFtpServer::start(bool automaticRecovery)
{
    if (m_running) return true;
    if (!activitySafe())
    {
        m_lastResult = "activity_state_not_safe";
        return false;
    }
    if (!m_storage.exists(WebRoot) && !m_storage.mkdir(WebRoot))
    {
        m_lastResult = "web_root_create_failed";
        return false;
    }
    File root = m_storage.open(WebRoot, FILE_READ);
    const bool rootReady = root && root.isDirectory();
    if (root) root.close();
    if (!rootReady)
    {
        m_lastResult = "web_root_unavailable";
        return false;
    }

    resetSession();
    m_controlServer.begin();
    m_controlServer.setNoDelay(true);
    m_running = true;
    m_automaticRecovery = automaticRecovery;
    m_lastResult = automaticRecovery ? "automatic_recovery_started" : "operator_started";
    return true;
}

void WebRecoveryFtpServer::stop(const char* result)
{
    abortTransfer(result, false);
    if (m_controlClient) m_controlClient.stop();
    m_controlServer.end();
    resetSession();
    m_running = false;
    m_automaticRecovery = false;
    m_lastResult = result != nullptr ? result : "stopped";
}

void WebRecoveryFtpServer::update(uint32_t nowMs)
{
    if (!m_running) return;

    // Emergency FTP is only a bootstrap path. Once the root entrypoint and
    // both production UI entrypoints exist, the site is usable and the
    // emergency server must not stay advertised as an active recovery
    // condition forever.
    if (m_automaticRecovery && webRootUsable())
    {
        stop("automatic_recovery_complete");
        return;
    }

    if (!activitySafe())
    {
        stop("stopped_activity_not_safe");
        return;
    }

    if (!m_controlClient || !m_controlClient.connected())
    {
        if (m_controlClient) m_controlClient.stop();
        abortTransfer("client_disconnected", false);
        resetSession();
        acceptControlClient(nowMs);
        return;
    }

    if (static_cast<uint32_t>(nowMs - m_lastClientActivityMs) > ClientTimeoutMs)
    {
        reply(421U, "Control connection timed out");
        m_controlClient.stop();
        abortTransfer("client_timeout", false);
        return;
    }

    updateTransfer(nowMs);
    if (m_transferState == TransferState::None) readControl(nowMs);
}

void WebRecoveryFtpServer::handleStatus()
{
    String response = F("{\"supported\":true,\"enabled\":");
    response += m_running ? F("true") : F("false");
    response += F(",\"automatic_recovery\":");
    response += m_automaticRecovery ? F("true") : F("false");
    response += F(",\"client_connected\":");
    response += clientConnected() ? F("true") : F("false");
    response += F(",\"transfer_active\":");
    response += transferActive() ? F("true") : F("false");
    response += F(",\"address\":\""); response += WiFi.softAPIP().toString();
    response += F("\",\"ap_address\":\""); response += WiFi.softAPIP().toString();
    response += F("\",\"sta_available\":");
    response += WiFi.status() == WL_CONNECTED ? F("true") : F("false");
    response += F(",\"sta_address\":\"");
    if (WiFi.status() == WL_CONNECTED) response += WiFi.localIP().toString();
    response += F("\",\"access_scope\":\"LOCAL_AP_OR_STA\"");
    response += F(",\"web_root_usable\":"); response += webRootUsable() ? F("true") : F("false");
    response += F(",\"port\":21,\"username\":\"CoilMaster\",\"root\":\"/web\",\"last_result\":\"");
    response += jsonEscaped(m_lastResult);
    response += F("\"}");
    m_webServer.send(200, "application/json; charset=utf-8", response);
}

void WebRecoveryFtpServer::handleStart()
{
    if (!start(false))
    {
        m_webServer.send(409, "application/json",
                         "{\"error\":\"ftp_start_blocked\"}");
        return;
    }
    m_webServer.send(200, "application/json", "{\"enabled\":true}");
}

void WebRecoveryFtpServer::handleStop()
{
    stop("operator_stopped");
    m_webServer.send(200, "application/json", "{\"enabled\":false}");
}

void WebRecoveryFtpServer::acceptControlClient(uint32_t nowMs)
{
    WiFiClient candidate = m_controlServer.available();
    if (!candidate) return;
    if (!sameLocalSubnet(candidate.remoteIP()))
    {
        candidate.print(F("421 Use CoilMaster AP or connected local network\r\n"));
        candidate.stop();
        m_lastResult = "non_local_client_rejected";
        return;
    }
    m_controlClient = candidate;
    m_controlClient.setNoDelay(true);
    m_lastClientActivityMs = nowMs;
    m_lastResult = "client_connected";
    reply(220U, "CoilMaster /web recovery FTP ready");
}

void WebRecoveryFtpServer::readControl(uint32_t nowMs)
{
    uint16_t processed = 0U;
    while (m_controlClient.available() > 0 && processed < 256U)
    {
        const char ch = static_cast<char>(m_controlClient.read());
        ++processed;
        m_lastClientActivityMs = nowMs;
        if (ch == '\n')
        {
            String line = m_commandBuffer;
            m_commandBuffer = "";
            line.trim();
            if (line.length() > 0U) processCommand(line, nowMs);
            if (m_transferState != TransferState::None) return;
        }
        else if (ch != '\r')
        {
            if (m_commandBuffer.length() >= MaxCommandLength)
            {
                m_commandBuffer = "";
                reply(500U, "Command too long");
            }
            else m_commandBuffer += ch;
        }
    }
}

void WebRecoveryFtpServer::processCommand(const String& line, uint32_t nowMs)
{
    const int separator = line.indexOf(' ');
    String command = separator >= 0 ? line.substring(0, separator) : line;
    String argument = separator >= 0 ? line.substring(static_cast<unsigned int>(separator + 1)) : String();
    command.toUpperCase();
    argument.trim();

    if (command == "USER")
    {
        m_userAccepted = argument == FtpUsername;
        m_authenticated = false;
        reply(m_userAccepted ? 331U : 530U,
              m_userAccepted ? "Password required" : "Invalid user");
        return;
    }
    if (command == "PASS")
    {
        m_authenticated = m_userAccepted && argument == FtpPassword;
        reply(m_authenticated ? 230U : 530U,
              m_authenticated ? "Logged in" : "Authentication failed");
        m_lastResult = m_authenticated ? "authenticated" : "authentication_failed";
        return;
    }
    if (command == "QUIT")
    {
        reply(221U, "Goodbye");
        m_controlClient.stop();
        return;
    }
    if (command == "NOOP") { reply(200U, "OK"); return; }
    if (command == "SYST") { reply(215U, "UNIX Type: L8"); return; }
    if (command == "FEAT")
    {
        m_controlClient.print(F("211-Features\r\n EPSV\r\n PASV\r\n SIZE\r\n UTF8\r\n211 End\r\n"));
        return;
    }
    if (command == "OPTS") { reply(200U, "UTF8 enabled"); return; }
    if (!loggedInOrReply()) return;

    if (command == "PWD")
    {
        reply(257U, '"' + m_currentDirectory + '"');
        return;
    }
    if (command == "TYPE") { reply(200U, "Type set"); return; }
    if (command == "PASV") { enterPassive(false); return; }
    if (command == "EPSV") { enterPassive(true); return; }

    String virtualPath;
    String storagePath;
    if (command == "CDUP")
    {
        const int slash = m_currentDirectory.lastIndexOf('/');
        m_currentDirectory = slash <= 0 ? "/" : m_currentDirectory.substring(0, slash);
        reply(250U, "Directory changed");
        return;
    }
    if (command == "CWD")
    {
        if (!resolvePath(argument, virtualPath, storagePath)) { reply(550U, "Invalid path"); return; }
        File directory = m_storage.open(storagePath, FILE_READ);
        const bool valid = directory && directory.isDirectory();
        if (directory) directory.close();
        if (!valid) { reply(550U, "Directory unavailable"); return; }
        m_currentDirectory = virtualPath;
        reply(250U, "Directory changed");
        return;
    }
    if (command == "SIZE")
    {
        if (!resolvePath(argument, virtualPath, storagePath)) { reply(550U, "Invalid path"); return; }
        File file = m_storage.open(storagePath, FILE_READ);
        if (!file || file.isDirectory()) { if (file) file.close(); reply(550U, "File unavailable"); return; }
        const size_t size = file.size(); file.close();
        reply(213U, String(static_cast<unsigned long>(size)));
        return;
    }
    if (command == "LIST" || command == "NLST")
    {
        String listArgument = argument;
        if (listArgument.startsWith("-")) listArgument = "";
        if (!resolvePath(listArgument, virtualPath, storagePath)) { reply(550U, "Invalid path"); return; }
        m_listDirectory = m_storage.open(storagePath, FILE_READ);
        if (!m_listDirectory || !m_listDirectory.isDirectory())
        {
            if (m_listDirectory) m_listDirectory.close();
            reply(550U, "Directory unavailable");
            return;
        }
        if (!openPassiveOrReply()) { m_listDirectory.close(); return; }
        m_listNamesOnly = command == "NLST";
        m_transferState = TransferState::AwaitList;
        m_dataDeadlineMs = nowMs + DataTimeoutMs;
        reply(150U, "Opening data connection");
        return;
    }
    if (command == "STOR")
    {
        if (!activitySafe()) { reply(450U, "Machine is not safely idle"); return; }
        if (!resolvePath(argument, virtualPath, storagePath) || virtualPath == "/")
        {
            reply(550U, "Invalid path"); return;
        }
        if (!openPassiveOrReply()) return;
        m_transferPath = storagePath;
        m_transferTemporaryPath = storagePath + F(".part");
        if (m_storage.exists(m_transferTemporaryPath)) m_storage.remove(m_transferTemporaryPath);
        m_transferFile = m_storage.open(m_transferTemporaryPath, FILE_WRITE);
        if (!m_transferFile) { closePassive(); reply(550U, "Cannot create temporary file"); return; }
        m_transferState = TransferState::AwaitStore;
        m_dataDeadlineMs = nowMs + DataTimeoutMs;
        reply(150U, "Opening data connection");
        return;
    }
    if (command == "MKD")
    {
        if (!activitySafe() || !resolvePath(argument, virtualPath, storagePath) || virtualPath == "/")
        { reply(550U, "Directory creation rejected"); return; }
        if (m_storage.exists(storagePath) || !m_storage.mkdir(storagePath))
        { reply(550U, "Directory creation failed"); return; }
        reply(257U, '"' + virtualPath + '"');
        return;
    }
    if (command == "DELE")
    {
        if (!activitySafe() || !resolvePath(argument, virtualPath, storagePath) || virtualPath == "/" ||
            !m_storage.remove(storagePath)) { reply(550U, "Delete failed"); return; }
        reply(250U, "File deleted");
        return;
    }
    if (command == "RMD")
    {
        if (!activitySafe() || !resolvePath(argument, virtualPath, storagePath) || virtualPath == "/" ||
            !m_storage.rmdir(storagePath)) { reply(550U, "Directory removal failed"); return; }
        reply(250U, "Directory removed");
        return;
    }
    if (command == "RNFR")
    {
        if (!resolvePath(argument, virtualPath, storagePath) || virtualPath == "/" || !m_storage.exists(storagePath))
        { reply(550U, "Source unavailable"); return; }
        m_renameFrom = storagePath;
        reply(350U, "Destination required");
        return;
    }
    if (command == "RNTO")
    {
        if (!activitySafe() || m_renameFrom.length() == 0U ||
            !resolvePath(argument, virtualPath, storagePath) || virtualPath == "/" ||
            m_storage.exists(storagePath) || !m_storage.rename(m_renameFrom, storagePath))
        { m_renameFrom = ""; reply(550U, "Rename failed"); return; }
        m_renameFrom = "";
        reply(250U, "Renamed");
        return;
    }
    reply(502U, "Command not implemented");
}

void WebRecoveryFtpServer::updateTransfer(uint32_t nowMs)
{
    if (m_transferState == TransferState::None) return;
    if (m_transferState == TransferState::AwaitStore ||
        m_transferState == TransferState::AwaitList)
    {
        WiFiClient candidate = m_dataServer.available();
        if (candidate)
        {
            if (!sameLocalSubnet(candidate.remoteIP()))
            {
                candidate.stop();
            }
            else
            {
                m_dataClient = candidate;
                m_transferState = m_transferState == TransferState::AwaitStore
                    ? TransferState::Storing : TransferState::Listing;
            }
        }
        if ((m_transferState == TransferState::AwaitStore || m_transferState == TransferState::AwaitList) &&
            static_cast<int32_t>(nowMs - m_dataDeadlineMs) >= 0)
        {
            abortTransfer("data_connection_timeout", true);
        }
        return;
    }

    if (m_transferState == TransferState::Storing)
    {
        uint8_t buffer[512];
        uint8_t chunks = 0U;
        while (m_dataClient.available() > 0 && chunks < 4U)
        {
            const int available = m_dataClient.available();
            const size_t requested = available < static_cast<int>(sizeof(buffer))
                ? static_cast<size_t>(available) : sizeof(buffer);
            const int read = m_dataClient.read(buffer, requested);
            if (read <= 0 || m_transferFile.write(buffer, static_cast<size_t>(read)) != static_cast<size_t>(read))
            {
                abortTransfer("storage_write_failed", true);
                return;
            }
            ++chunks;
            m_lastClientActivityMs = nowMs;
        }
        if (!m_dataClient.connected() && m_dataClient.available() == 0) finishStore();
        return;
    }

    if (m_transferState == TransferState::Listing)
    {
        if (!m_dataClient.connected()) { abortTransfer("data_client_disconnected", true); return; }
        if (!sendListEntry()) finishList();
    }
}

void WebRecoveryFtpServer::abortTransfer(const char* result, bool sendReply)
{
    if (m_transferFile) m_transferFile.close();
    if (m_listDirectory) m_listDirectory.close();
    if (m_dataClient) m_dataClient.stop();
    if (m_transferTemporaryPath.length() > 0U && m_storage.exists(m_transferTemporaryPath))
        m_storage.remove(m_transferTemporaryPath);
    closePassive();
    m_transferState = TransferState::None;
    m_transferPath = "";
    m_transferTemporaryPath = "";
    m_lastResult = result != nullptr ? result : "transfer_aborted";
    if (sendReply && m_controlClient && m_controlClient.connected())
        reply(426U, "Transfer aborted");
}

void WebRecoveryFtpServer::finishStore()
{
    m_transferFile.flush();
    m_transferFile.close();
    if (m_dataClient) m_dataClient.stop();
    closePassive();

    const String backupPath = m_transferPath + F(".bak");
    if (m_storage.exists(backupPath)) m_storage.remove(backupPath);
    const bool hadTarget = m_storage.exists(m_transferPath);
    if (hadTarget && !m_storage.rename(m_transferPath, backupPath))
    {
        abortTransfer("target_backup_failed", true);
        return;
    }
    if (!m_storage.rename(m_transferTemporaryPath, m_transferPath))
    {
        if (hadTarget) m_storage.rename(backupPath, m_transferPath);
        abortTransfer("atomic_rename_failed", true);
        return;
    }
    if (hadTarget) m_storage.remove(backupPath);
    m_transferState = TransferState::None;
    m_transferPath = "";
    m_transferTemporaryPath = "";
    m_lastResult = "upload_completed";
    reply(226U, "Transfer complete");
}

void WebRecoveryFtpServer::finishList()
{
    if (m_listDirectory) m_listDirectory.close();
    if (m_dataClient) m_dataClient.stop();
    closePassive();
    m_transferState = TransferState::None;
    m_lastResult = "listing_completed";
    reply(226U, "Transfer complete");
}

void WebRecoveryFtpServer::resetSession()
{
    closePassive();
    m_commandBuffer = "";
    m_currentDirectory = "/";
    m_transferPath = "";
    m_transferTemporaryPath = "";
    m_renameFrom = "";
    m_userAccepted = false;
    m_authenticated = false;
    m_transferState = TransferState::None;
}

void WebRecoveryFtpServer::closePassive()
{
    if (m_dataClient) m_dataClient.stop();
    if (m_passiveOpen) m_dataServer.end();
    m_passiveOpen = false;
}

bool WebRecoveryFtpServer::activitySafe() const
{
    return m_activityProbe != nullptr &&
           m_activityProbe() == BackupActivityCheck::Safe;
}

bool WebRecoveryFtpServer::loggedInOrReply()
{
    if (m_authenticated) return true;
    reply(530U, "Login required");
    return false;
}

bool WebRecoveryFtpServer::openPassiveOrReply()
{
    if (m_passiveOpen) return true;
    reply(425U, "Use PASV or EPSV first");
    return false;
}

bool WebRecoveryFtpServer::resolvePath(const String& argument,
                                       String& virtualPath,
                                       String& storagePath) const
{
    String source = argument;
    source.trim();
    if (source.length() == 0U) source = m_currentDirectory;
    else if (!source.startsWith("/"))
        source = (m_currentDirectory == "/" ? "/" : m_currentDirectory + '/') + source;
    if (source.length() >= MaxPathLength || source.indexOf('\\') >= 0) return false;

    String normalized = "/";
    size_t start = 0U;
    while (start < source.length())
    {
        while (start < source.length() && source[start] == '/') ++start;
        if (start >= source.length()) break;
        const int nextSlash = source.indexOf('/', static_cast<unsigned int>(start));
        const size_t end = nextSlash < 0 ? source.length() : static_cast<size_t>(nextSlash);
        const String segment = source.substring(static_cast<unsigned int>(start), static_cast<unsigned int>(end));
        if (segment == ".." || segment.indexOf('\r') >= 0 || segment.indexOf('\n') >= 0 || segment.length() == 0U)
            return false;
        if (segment != ".")
        {
            if (normalized.length() > 1U) normalized += '/';
            normalized += segment;
        }
        start = end + 1U;
    }
    if (normalized.length() >= MaxPathLength) return false;
    virtualPath = normalized;
    storagePath = String(WebRoot) + (normalized == "/" ? "" : normalized);
    return true;
}

bool WebRecoveryFtpServer::sameLocalSubnet(const IPAddress& address) const
{
    const IPAddress ap = WiFi.softAPIP();
    if (address[0] == ap[0] && address[1] == ap[1] && address[2] == ap[2])
        return true;

    if (WiFi.status() != WL_CONNECTED) return false;
    const IPAddress sta = WiFi.localIP();
    const IPAddress mask = WiFi.subnetMask();
    for (uint8_t i = 0U; i < 4U; ++i)
    {
        if ((address[i] & mask[i]) != (sta[i] & mask[i])) return false;
    }
    return true;
}

bool WebRecoveryFtpServer::webRootUsable() const
{
    return m_storage.exists("/web/index.html") &&
           m_storage.exists("/web/desktop/index.html") &&
           m_storage.exists("/web/mobile/index.html");
}

IPAddress WebRecoveryFtpServer::passiveAddressFor(const IPAddress& remote) const
{
    const IPAddress ap = WiFi.softAPIP();
    if (remote[0] == ap[0] && remote[1] == ap[1] && remote[2] == ap[2])
        return ap;
    return WiFi.status() == WL_CONNECTED ? WiFi.localIP() : ap;
}

bool WebRecoveryFtpServer::sendListEntry()
{
    File entry = m_listDirectory.openNextFile();
    if (!entry) return false;
    String name = baseName(entry.name());
    name.replace("\r", "_");
    name.replace("\n", "_");
    if (m_listNamesOnly)
    {
        m_dataClient.print(name); m_dataClient.print(F("\r\n"));
    }
    else
    {
        m_dataClient.print(entry.isDirectory() ? 'd' : '-');
        m_dataClient.print(F("rw-r--r-- 1 coilmaster coilmaster "));
        m_dataClient.print(static_cast<unsigned long>(entry.size()));
        m_dataClient.print(F(" Jan 01 00:00 "));
        m_dataClient.print(name);
        m_dataClient.print(F("\r\n"));
    }
    entry.close();
    return true;
}

void WebRecoveryFtpServer::reply(uint16_t code, const String& text)
{
    if (!m_controlClient) return;
    m_controlClient.print(code);
    m_controlClient.print(' ');
    m_controlClient.print(text);
    m_controlClient.print(F("\r\n"));
}

void WebRecoveryFtpServer::enterPassive(bool extended)
{
    closePassive();
    m_dataServer.begin();
    m_dataServer.setNoDelay(true);
    m_passiveOpen = true;
    if (extended)
    {
        reply(229U, "Entering Extended Passive Mode (|||50009|)");
        return;
    }
    const IPAddress address = passiveAddressFor(m_controlClient.remoteIP());
    String response = F("Entering Passive Mode (");
    response += address[0]; response += ','; response += address[1]; response += ',';
    response += address[2]; response += ','; response += address[3];
    response += F(",195,89)");
    reply(227U, response);
}

bool WebRecoveryFtpServer::running() const { return m_running; }
bool WebRecoveryFtpServer::automaticRecovery() const { return m_automaticRecovery; }
bool WebRecoveryFtpServer::clientConnected()
{
    return m_controlClient && m_controlClient.connected();
}
bool WebRecoveryFtpServer::transferActive() const
{
    return m_transferState != TransferState::None;
}
const String& WebRecoveryFtpServer::lastResult() const { return m_lastResult; }
}