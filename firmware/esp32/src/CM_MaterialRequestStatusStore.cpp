#include "CM_MaterialRequestStatusStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}
}

MaterialRequestStatusStore::MaterialRequestStatusStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MaterialRequestStatusStore::begin()
{
    m_ready = false;
    if (!ensureDirectory() || !validateStatusFileStructure()) return false;
    m_ready = true;
    return true;
}

bool MaterialRequestStatusStore::ready() const
{
    return m_ready;
}

bool MaterialRequestStatusStore::resolve(uint32_t materialRequestId,
                                         MaterialRequestStatusState& state,
                                         bool& found) const
{
    state = MaterialRequestStatusState();
    state.materialRequestId = materialRequestId;
    found = false;
    if (!ready() || materialRequestId == 0UL) return false;

    if (!requestExists(materialRequestId, found) || !found) return found;
    state.status = "DRAFT";

    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousTransitionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t transitionId = 0UL;
        uint32_t requestId = 0UL;
        String fromStatus;
        String toStatus;
        String changedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "transition_id", transitionId) || transitionId == 0UL ||
            transitionId <= previousTransitionId ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            !findString(line, "from_status", fromStatus) ||
            !findString(line, "to_status", toStatus) ||
            !findString(line, "changed_at", changedAt) || changedAt.length() < 10U ||
            changedAt.length() > 32U || !validTransition(fromStatus, toStatus))
        {
            file.close();
            return false;
        }
        previousTransitionId = transitionId;
        if (requestId != materialRequestId) continue;
        if (fromStatus != state.status || state.transitionCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        state.status = toStatus;
        ++state.transitionCount;
    }
    file.close();
    return true;
}

bool MaterialRequestStatusStore::transition(uint32_t materialRequestId,
                                            const String& targetStatus,
                                            const String& changedAt,
                                            uint32_t& transitionId)
{
    transitionId = 0UL;
    if (!ready() || materialRequestId == 0UL || !validStatus(targetStatus) ||
        changedAt.length() < 10U || changedAt.length() > 32U)
    {
        return false;
    }

    MaterialRequestStatusState state;
    bool found = false;
    if (!resolve(materialRequestId, state, found) || !found ||
        !validTransition(state.status, targetStatus) ||
        !nextTransitionId(transitionId))
    {
        transitionId = 0UL;
        return false;
    }

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        transitionId = 0UL;
        return false;
    }

    String line;
    line.reserve(240U);
    line = F("{\"transition_id\":"); line += transitionId;
    line += F(",\"material_request_id\":"); line += materialRequestId;
    line += F(",\"from_status\":\""); line += state.status;
    line += F("\",\"to_status\":\""); line += targetStatus;
    line += F("\",\"changed_at\":\""); line += jsonEscape(changedAt);
    line += F("\"}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        transitionId = 0UL;
        return false;
    }
    return true;
}

bool MaterialRequestStatusStore::validStatus(const String& status)
{
    return status == "DRAFT" || status == "ISSUED" || status == "PRICED" ||
           status == "CLOSED";
}

bool MaterialRequestStatusStore::validTransition(const String& fromStatus,
                                                 const String& toStatus)
{
    return (fromStatus == "DRAFT" && toStatus == "ISSUED") ||
           (fromStatus == "ISSUED" && toStatus == "PRICED") ||
           (fromStatus == "PRICED" && toStatus == "CLOSED");
}

bool MaterialRequestStatusStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool MaterialRequestStatusStore::requestExists(uint32_t materialRequestId,
                                               bool& found) const
{
    found = false;
    if (!m_storage.exists(RequestsPath)) return true;
    File file = m_storage.open(RequestsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t requestId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            requestId <= previousId)
        {
            file.close();
            return false;
        }
        previousId = requestId;
        if (requestId != materialRequestId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();
    return true;
}

bool MaterialRequestStatusStore::nextTransitionId(uint32_t& transitionId) const
{
    transitionId = 1UL;
    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "transition_id", currentId) || currentId == 0UL ||
            currentId <= previousId)
        {
            file.close();
            return false;
        }
        previousId = currentId;
    }
    file.close();
    if (previousId == 0xFFFFFFFFUL) return false;
    transitionId = previousId + 1UL;
    return true;
}

bool MaterialRequestStatusStore::validateStatusFileStructure() const
{
    if (!m_storage.exists(Path)) return true;
    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousTransitionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t transitionId = 0UL;
        uint32_t requestId = 0UL;
        String fromStatus;
        String toStatus;
        String changedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "transition_id", transitionId) || transitionId == 0UL ||
            transitionId <= previousTransitionId ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            !findString(line, "from_status", fromStatus) ||
            !findString(line, "to_status", toStatus) ||
            !findString(line, "changed_at", changedAt) || changedAt.length() < 10U ||
            changedAt.length() > 32U || !validTransition(fromStatus, toStatus))
        {
            file.close();
            return false;
        }
        previousTransitionId = transitionId;
    }
    file.close();
    return true;
}

bool MaterialRequestStatusStore::findUnsigned(const String& line,
                                              const char* key,
                                              uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
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

bool MaterialRequestStatusStore::findString(const String& line,
                                            const char* key,
                                            String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (!escaped && ch == '"')
            return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (!escaped && ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (escaped)
        {
            if (ch == '"' || ch == '\\') value += ch;
            else if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else return false;
            escaped = false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

String MaterialRequestStatusStore::jsonEscape(const String& value)
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
