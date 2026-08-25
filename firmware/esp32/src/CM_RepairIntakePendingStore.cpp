#include "CM_RepairIntakePendingStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
RepairIntakePendingStore::RepairIntakePendingStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool RepairIntakePendingStore::begin()
{
    m_ready = false;
    if (!ensureDirectory() || !recoverTemp()) return false;
    if (m_storage.exists(Path))
    {
        RepairIntakePending pending;
        if (!loadPath(Path, pending)) return false;
    }
    m_ready = true;
    return true;
}

bool RepairIntakePendingStore::ready() const
{
    return m_ready;
}

bool RepairIntakePendingStore::hasPending() const
{
    return ready() && m_storage.exists(Path);
}

bool RepairIntakePendingStore::save(const RepairIntakePending& pending)
{
    if (!ready() || !pending.valid() || m_storage.exists(Path)) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;

    String line;
    line.reserve(320U);
    line = F("{\"repair_id\":"); line += pending.repairId;
    line += F(",\"client_id\":"); line += pending.clientId;
    line += F(",\"motor_id\":"); line += pending.motorId;
    line += F(",\"source_winding_version_id\":"); line += pending.sourceWindingVersionId;
    line += F(",\"received_at\":\""); line += jsonEscape(pending.receivedAt);
    line += F("\",\"source_kind\":\""); line += jsonEscape(pending.sourceKind);
    line += F("\"}\n");

    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length()) return false;

    RepairIntakePending verified;
    if (!loadPath(TempPath, verified) ||
        verified.repairId != pending.repairId ||
        verified.clientId != pending.clientId ||
        verified.motorId != pending.motorId ||
        verified.sourceWindingVersionId != pending.sourceWindingVersionId ||
        verified.receivedAt != pending.receivedAt ||
        verified.sourceKind != pending.sourceKind)
    {
        return false;
    }

    return m_storage.rename(TempPath, Path);
}

bool RepairIntakePendingStore::load(RepairIntakePending& pending, bool& found) const
{
    pending = RepairIntakePending();
    found = false;
    if (!ready()) return false;
    if (!m_storage.exists(Path)) return true;
    if (!loadPath(Path, pending)) return false;
    found = true;
    return true;
}

bool RepairIntakePendingStore::clear()
{
    if (!ready()) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
    if (!m_storage.exists(Path)) return true;
    return m_storage.remove(Path);
}

bool RepairIntakePendingStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool RepairIntakePendingStore::recoverTemp()
{
    const bool mainExists = m_storage.exists(Path);
    const bool tempExists = m_storage.exists(TempPath);
    if (!tempExists) return true;

    RepairIntakePending mainPending;
    if (mainExists && loadPath(Path, mainPending))
    {
        return m_storage.remove(TempPath);
    }

    RepairIntakePending tempPending;
    if (!loadPath(TempPath, tempPending)) return false;
    if (mainExists) return false;
    return m_storage.rename(TempPath, Path);
}

bool RepairIntakePendingStore::loadPath(const char* path,
                                        RepairIntakePending& pending) const
{
    pending = RepairIntakePending();
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t rawSize = file.size();
    if (rawSize == 0U || rawSize > 1024U)
    {
        file.close();
        return false;
    }
    String line = file.readStringUntil('\n');
    const bool extraData = file.available();
    file.close();
    if (extraData || line.length() == 0U || !FlatJsonObjectValidator::valid(line))
        return false;

    if (!findUnsigned(line, "repair_id", pending.repairId) ||
        !findUnsigned(line, "client_id", pending.clientId) ||
        !findUnsigned(line, "motor_id", pending.motorId) ||
        !findUnsigned(line, "source_winding_version_id", pending.sourceWindingVersionId) ||
        !findString(line, "received_at", pending.receivedAt) ||
        !findString(line, "source_kind", pending.sourceKind) ||
        !pending.valid())
    {
        pending = RepairIntakePending();
        return false;
    }
    return true;
}

bool RepairIntakePendingStore::findUnsigned(const String& line,
                                            const char* key,
                                            uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    uint32_t parsed = 0UL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

bool RepairIntakePendingStore::findString(const String& line,
                                          const char* key,
                                          String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    const int valueStart = start + marker.length();
    int valueEnd = valueStart;
    bool escaped = false;
    for (; valueEnd < line.length(); ++valueEnd)
    {
        const char ch = line[valueEnd];
        if (!escaped && ch == '"') break;
        if (!escaped && ch == '\\') escaped = true;
        else escaped = false;
    }
    if (valueEnd >= line.length() || valueEnd + 1 >= line.length() ||
        (line[valueEnd + 1] != ',' && line[valueEnd + 1] != '}'))
    {
        return false;
    }
    value = line.substring(valueStart, valueEnd);
    return true;
}

String RepairIntakePendingStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"')
        {
            escaped += '\\';
            escaped += ch;
        }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
}
