#include "CM_MaterialRequestStore.h"

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

MaterialRequestStore::MaterialRequestStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MaterialRequestStore::begin()
{
    m_ready = false;
    if (!ensureDirectory()) return false;
    if (m_storage.exists(Path))
    {
        File file = m_storage.open(Path, FILE_READ);
        if (!prepareNdjson(file))
        {
            if (file) file.close();
            return false;
        }
        file.close();
    }
    m_ready = true;
    return true;
}

bool MaterialRequestStore::ready() const
{
    return m_ready;
}

bool MaterialRequestStore::append(const NewMaterialRequest& request,
                                  uint32_t& requestId)
{
    requestId = 0UL;
    if (!ready() || request.repairId == 0UL || request.clientId == 0UL ||
        request.motorId == 0UL || request.createdAt.length() < 10U ||
        request.createdAt.length() > 32U || request.comment.length() > 500U ||
        !nextRequestId(requestId))
    {
        return false;
    }

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(720U);
    line = F("{\"material_request_id\":"); line += requestId;
    line += F(",\"repair_id\":"); line += request.repairId;
    line += F(",\"client_id\":"); line += request.clientId;
    line += F(",\"motor_id\":"); line += request.motorId;
    line += F(",\"initial_status\":\"DRAFT\",\"created_at\":\"");
    line += jsonEscape(request.createdAt);
    line += '"';
    if (request.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(request.comment);
        line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool MaterialRequestStore::appendByIdJson(String& json,
                                          uint32_t requestId,
                                          bool& found) const
{
    found = false;
    if (!ready() || requestId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_request_id", id) || id == 0UL ||
            id <= previous)
        {
            file.close();
            return false;
        }
        previous = id;
        if (id == requestId)
        {
            if (found)
            {
                file.close();
                return false;
            }
            json += line;
            found = true;
        }
    }
    file.close();
    return true;
}

bool MaterialRequestStore::appendRepairPageJson(String& json,
                                                uint32_t repairId,
                                                uint32_t cursor,
                                                uint8_t limit,
                                                uint16_t& count,
                                                uint32_t& nextCursor,
                                                bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || repairId == 0UL || limit == 0U || limit > MaxPageSize)
        return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL;
        uint32_t currentRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_request_id", id) || id == 0UL ||
            id <= previous ||
            !findUnsigned(line, "repair_id", currentRepairId) || currentRepairId == 0UL)
        {
            file.close();
            return false;
        }
        previous = id;
        if (id <= cursor || currentRepairId != repairId) continue;
        if (count >= limit)
        {
            hasMore = true;
            break;
        }
        if (count > 0U) json += ',';
        json += line;
        ++count;
        nextCursor = id;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}

bool MaterialRequestStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool MaterialRequestStore::nextRequestId(uint32_t& requestId) const
{
    requestId = 1UL;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t current = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_request_id", current) || current == 0UL ||
            current <= previous)
        {
            file.close();
            return false;
        }
        previous = current;
    }
    file.close();
    if (previous == 0xFFFFFFFFUL) return false;
    requestId = previous + 1UL;
    return true;
}

bool MaterialRequestStore::findUnsigned(const String& line,
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
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}'))
        return false;
    value = parsed;
    return true;
}

String MaterialRequestStore::jsonEscape(const String& value)
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
