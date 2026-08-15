#include "CM_RemoteBackupTransfer.h"

#include <WiFi.h>

namespace CM
{
namespace
{
constexpr uint32_t ReplyTimeoutMs = 5000UL;
constexpr size_t TransferChunkSize = 512U;

bool parseSizeReply(const String& line, uint32_t& size)
{
    size = 0UL;
    if (!line.startsWith("213 ")) return false;
    const String value = line.substring(4);
    if (value.length() == 0U) return false;
    for (size_t i = 0U; i < value.length(); ++i)
    {
        if (!isDigit(value[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(value[i] - '0');
        if (size > (0xFFFFFFFFUL - digit) / 10UL) return false;
        size = size * 10UL + digit;
    }
    return true;
}
}

RemoteBackupTransfer::RemoteBackupTransfer(fs::FS& storage)
    : m_storage(storage), m_activityProbe(nullptr),
      m_operation(Operation::Upload), m_phase(Phase::Idle),
      m_deadlineMs(0UL), m_totalBytes(0UL), m_sentBytes(0UL) {}

void RemoteBackupTransfer::setActivityProbe(
    BackupActivityGuard::RuntimeProbe activityProbe)
{
    m_activityProbe = activityProbe;
}

bool RemoteBackupTransfer::start(const RemoteBackupSettings& settings,
                                 const String& logicalName,
                                 const String& localPath,
                                 const String& remoteName)
{
    if (active() || !RemoteBackupSettingsStore::valid(settings) ||
        !settings.enabled || WiFi.status() != WL_CONNECTED ||
        logicalName.length() == 0U || remoteName.length() == 0U ||
        remoteName.indexOf('/') >= 0 || remoteName.indexOf("..") >= 0 ||
        !m_storage.exists(localPath)) return false;

    m_file = m_storage.open(localPath, FILE_READ);
    if (!m_file || m_file.isDirectory())
    {
        if (m_file) m_file.close();
        return false;
    }
    m_settings = settings;
    m_operation = Operation::Upload;
    m_logicalName = logicalName;
    m_remoteName = remoteName;
    m_tempName = remoteName + F(".part");
    m_totalBytes = static_cast<uint32_t>(m_file.size());
    m_sentBytes = 0UL;
    m_error = String();
    m_replyLine = String();
    m_control.setTimeout(300UL);
    if (!m_control.connect(settings.host.c_str(), settings.port, 300))
    {
        fail("ftp_connect_failed");
        return false;
    }
    m_phase = Phase::Greeting;
    resetDeadline(millis());
    return true;
}

bool RemoteBackupTransfer::startDelete(const RemoteBackupSettings& settings,
                                       const String& logicalName,
                                       const String& remoteName)
{
    if (active() || !RemoteBackupSettingsStore::valid(settings) ||
        !settings.enabled || WiFi.status() != WL_CONNECTED ||
        logicalName.length() == 0U || remoteName.length() == 0U ||
        remoteName.indexOf('/') >= 0 || remoteName.indexOf("..") >= 0)
        return false;
    m_settings = settings;
    m_operation = Operation::Delete;
    m_logicalName = logicalName;
    m_remoteName = remoteName;
    m_tempName = String();
    m_totalBytes = 0UL;
    m_sentBytes = 0UL;
    m_error = String();
    m_replyLine = String();
    m_control.setTimeout(300UL);
    if (!m_control.connect(settings.host.c_str(), settings.port, 300))
    {
        fail("ftp_connect_failed");
        return false;
    }
    m_phase = Phase::Greeting;
    resetDeadline(millis());
    return true;
}

void RemoteBackupTransfer::update(uint32_t nowMs)
{
    if (!active()) return;
    if (m_activityProbe == nullptr ||
        m_activityProbe() != BackupActivityCheck::Safe)
    {
        fail("winding_became_active_or_unavailable");
        return;
    }
    if (m_phase != Phase::Sending &&
        static_cast<int32_t>(nowMs - m_deadlineMs) >= 0)
    {
        fail("ftp_reply_timeout");
        return;
    }

    if (m_phase == Phase::Sending)
    {
        if (!m_file.available())
        {
            m_file.close();
            m_data.stop();
            m_phase = Phase::StoreComplete;
            resetDeadline(nowMs);
            return;
        }
        uint8_t buffer[TransferChunkSize];
        const size_t count = m_file.read(buffer, sizeof(buffer));
        if (count == 0U || m_data.write(buffer, count) != count)
        {
            fail("ftp_data_write_failed");
            return;
        }
        m_sentBytes += static_cast<uint32_t>(count);
        return;
    }

    uint16_t code = 0U;
    String reply;
    if (!readReply(code, reply)) return;

    switch (m_phase)
    {
        case Phase::Greeting:
            if (code != 220U || !sendCommand(String(F("USER ")) + m_settings.username,
                                              Phase::User, nowMs))
                fail("ftp_greeting_failed");
            break;
        case Phase::User:
            if (code == 230U)
            {
                if (!sendCommand(String(F("CWD ")) + m_settings.remoteDirectory,
                                 Phase::ChangeDirectory, nowMs))
                    fail("ftp_command_failed");
            }
            else if (code == 331U)
            {
                if (!sendCommand(String(F("PASS ")) + m_settings.password,
                                 Phase::Password, nowMs))
                    fail("ftp_command_failed");
            }
            else fail("ftp_authentication_failed");
            break;
        case Phase::Password:
            if (code != 230U ||
                !sendCommand(String(F("CWD ")) + m_settings.remoteDirectory,
                             Phase::ChangeDirectory, nowMs))
                fail("ftp_authentication_failed");
            break;
        case Phase::ChangeDirectory:
            if (code < 200U || code >= 300U)
                fail("ftp_remote_directory_unavailable");
            else if (m_operation == Operation::Delete)
            {
                if (!sendCommand(String(F("DELE ")) + m_remoteName,
                                 Phase::DeleteOnly, nowMs))
                    fail("ftp_command_failed");
            }
            else if (!sendCommand(F("TYPE I"), Phase::BinaryMode, nowMs))
                fail("ftp_command_failed");
            break;
        case Phase::BinaryMode:
            if (code < 200U || code >= 300U ||
                !sendCommand(String(F("DELE ")) + m_tempName,
                             Phase::DeleteTemp, nowMs))
                fail("ftp_binary_mode_failed");
            break;
        case Phase::DeleteTemp:
            if (!sendCommand(F("PASV"), Phase::Passive, nowMs))
                fail("ftp_command_failed");
            break;
        case Phase::Passive:
            if (code != 227U || !openPassiveData(reply, nowMs))
                fail("ftp_passive_mode_failed");
            break;
        case Phase::StoreReady:
            if (code != 125U && code != 150U)
                fail("ftp_store_rejected");
            else
                m_phase = Phase::Sending;
            break;
        case Phase::StoreComplete:
            if (code != 226U ||
                !sendCommand(String(F("SIZE ")) + m_tempName,
                             Phase::Size, nowMs))
                fail("ftp_store_incomplete");
            break;
        case Phase::Size:
        {
            uint32_t remoteSize = 0UL;
            if (code != 213U || !parseSizeReply(reply, remoteSize) ||
                remoteSize != m_totalBytes || m_sentBytes != m_totalBytes ||
                !sendCommand(String(F("DELE ")) + m_remoteName,
                             Phase::DeleteFinal, nowMs))
                fail("ftp_size_verification_failed");
            break;
        }
        case Phase::DeleteFinal:
            // A missing previous final file is a normal FTP 550 response.
            if (!sendCommand(String(F("RNFR ")) + m_tempName,
                             Phase::RenameFrom, nowMs))
                fail("ftp_command_failed");
            break;
        case Phase::RenameFrom:
            if (code != 350U ||
                !sendCommand(String(F("RNTO ")) + m_remoteName,
                             Phase::RenameTo, nowMs))
                fail("ftp_rename_from_failed");
            break;
        case Phase::RenameTo:
            if (code < 200U || code >= 300U)
                fail("ftp_rename_failed");
            else
            {
                m_control.print(F("QUIT\r\n"));
                closeTransfer();
                m_phase = Phase::Complete;
            }
            break;
        case Phase::DeleteOnly:
            // Retention deletion is idempotent: 550 means the exact managed
            // file is already absent, which is safe after an interrupted pass.
            if ((code < 200U || code >= 300U) && code != 550U)
                fail("ftp_delete_failed");
            else
            {
                m_control.print(F("QUIT\r\n"));
                closeTransfer();
                m_phase = Phase::Complete;
            }
            break;
        case Phase::Idle:
        case Phase::Sending:
        case Phase::Complete:
        case Phase::Failed:
            break;
    }
}

bool RemoteBackupTransfer::active() const
{
    return m_phase != Phase::Idle && m_phase != Phase::Complete &&
           m_phase != Phase::Failed;
}

bool RemoteBackupTransfer::succeeded() const { return m_phase == Phase::Complete; }

const char* RemoteBackupTransfer::stateName() const
{
    if (m_phase == Phase::Idle) return "IDLE";
    if (m_phase == Phase::Complete) return "COMPLETED";
    if (m_phase == Phase::Failed) return "FAILED";
    if (m_phase == Phase::Sending) return "UPLOADING";
    return "FTP_NEGOTIATION";
}

const String& RemoteBackupTransfer::error() const { return m_error; }
const String& RemoteBackupTransfer::logicalName() const { return m_logicalName; }
const String& RemoteBackupTransfer::remoteName() const { return m_remoteName; }
uint32_t RemoteBackupTransfer::bytesTotal() const { return m_totalBytes; }
uint32_t RemoteBackupTransfer::bytesSent() const { return m_sentBytes; }

bool RemoteBackupTransfer::readReply(uint16_t& code, String& line)
{
    code = 0U;
    while (m_control.available())
    {
        const char ch = static_cast<char>(m_control.read());
        if (ch == '\r') continue;
        if (ch != '\n')
        {
            if (m_replyLine.length() >= 255U) { fail("ftp_reply_too_long"); return false; }
            m_replyLine += ch;
            continue;
        }
        line = m_replyLine;
        m_replyLine = String();
        if (line.length() >= 4U && isDigit(line[0]) && isDigit(line[1]) &&
            isDigit(line[2]) && line[3] == ' ')
        {
            code = static_cast<uint16_t>((line[0] - '0') * 100 +
                                         (line[1] - '0') * 10 +
                                         (line[2] - '0'));
            return true;
        }
    }
    return false;
}

bool RemoteBackupTransfer::sendCommand(const String& command,
                                       Phase next,
                                       uint32_t nowMs)
{
    if (!m_control.connected() || m_control.print(command) != command.length() ||
        m_control.print("\r\n") != 2U) return false;
    m_phase = next;
    resetDeadline(nowMs);
    return true;
}

bool RemoteBackupTransfer::openPassiveData(const String& reply, uint32_t nowMs)
{
    const int open = reply.indexOf('('), close = reply.indexOf(')', open + 1);
    if (open < 0 || close < 0) return false;
    uint16_t parts[6] = {};
    int cursor = open + 1;
    for (uint8_t i = 0U; i < 6U; ++i)
    {
        int end = i == 5U ? close : reply.indexOf(',', cursor);
        if (end < 0) return false;
        const String item = reply.substring(cursor, end);
        if (item.length() == 0U) return false;
        uint16_t value = 0U;
        for (size_t j = 0U; j < item.length(); ++j)
        {
            if (!isDigit(item[j])) return false;
            value = static_cast<uint16_t>(value * 10U + (item[j] - '0'));
            if (value > 255U) return false;
        }
        parts[i] = value;
        cursor = end + 1;
    }
    // Ignore the advertised host to prevent FTP bounce to a third-party host.
    // The passive data socket must return to the configured control server.
    IPAddress address = m_control.remoteIP();
    const uint16_t port = static_cast<uint16_t>(parts[4] * 256U + parts[5]);
    if (port == 0U || !m_data.connect(address, port, 300)) return false;
    return sendCommand(String(F("STOR ")) + m_tempName,
                       Phase::StoreReady, nowMs);
}

void RemoteBackupTransfer::fail(const char* reason)
{
    m_error = reason != nullptr ? reason : "remote_backup_failed";
    closeTransfer();
    m_phase = Phase::Failed;
}

void RemoteBackupTransfer::closeTransfer()
{
    if (m_file) m_file.close();
    m_data.stop();
    m_control.stop();
}

void RemoteBackupTransfer::resetDeadline(uint32_t nowMs)
{
    m_deadlineMs = nowMs + ReplyTimeoutMs;
}
}
