#include "CM_SpoolMaterialBridgeStore.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
constexpr uint8_t BridgeAuditBatchSize = 24U;
}

SpoolMaterialBridgeStore::SpoolMaterialBridgeStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool SpoolMaterialBridgeStore::begin()
{
    m_ready = false;
    if (!ensureDirectory()) return false;
    if (!m_storage.exists(Path))
    {
        File created = m_storage.open(Path, FILE_WRITE);
        if (!created) return false;
        created.close();
    }
    if (!validateAll()) return false;
    m_ready = true;
    return true;
}

bool SpoolMaterialBridgeStore::ready() const
{
    return m_ready;
}

bool SpoolMaterialBridgeStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/warehouse") && !m_storage.mkdir("/data/warehouse"))
        return false;
    return true;
}

bool SpoolMaterialBridgeStore::validNew(const NewSpoolMaterialBridge& bridge)
{
    return bridge.spoolId != 0UL && bridge.warehouseItemId != 0UL &&
           (bridge.wireType == "CU" || bridge.wireType == "AL") &&
           bridge.diameterHundredthsMm != 0U &&
           bridge.linkedAt.length() >= 10U && bridge.linkedAt.length() <= 32U;
}

bool SpoolMaterialBridgeStore::append(const NewSpoolMaterialBridge& source,
                                      uint32_t& bridgeId)
{
    bridgeId = 0UL;
    if (!m_ready || !validNew(source)) return false;

    SpoolMaterialBridge existing;
    bool found = false;
    if (!analyzeBySpool(source.spoolId, existing, found, &bridgeId) ||
        found || bridgeId == 0UL)
    {
        return false;
    }

    String line;
    line.reserve(256U);
    line = F("{\"bridge_id\":"); line += bridgeId;
    line += F(",\"spool_id\":"); line += source.spoolId;
    line += F(",\"warehouse_item_id\":"); line += source.warehouseItemId;
    line += F(",\"wire_type\":\""); line += source.wireType;
    line += F("\",\"diameter_hundredths_mm\":"); line += source.diameterHundredthsMm;
    line += F(",\"linked_at\":\""); line += jsonEscape(source.linkedAt);
    line += F("\"}\n");

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
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

bool SpoolMaterialBridgeStore::loadBySpool(uint32_t spoolId,
                                           SpoolMaterialBridge& bridge,
                                           bool& found) const
{
    return analyzeBySpool(spoolId, bridge, found, nullptr);
}

bool SpoolMaterialBridgeStore::analyzeBySpool(uint32_t spoolId,
                                              SpoolMaterialBridge& bridge,
                                              bool& found,
                                              uint32_t* nextBridgeId) const
{
    bridge = SpoolMaterialBridge();
    found = false;
    if (nextBridgeId != nullptr) *nextBridgeId = 0UL;
    if (!m_ready || spoolId == 0UL) return false;

    File file = m_storage.open(Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        SpoolMaterialBridge parsed;
        if (!parse(line, parsed) || parsed.bridgeId <= previousId)
        {
            file.close();
            return false;
        }
        previousId = parsed.bridgeId;
        if (parsed.spoolId != spoolId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        bridge = parsed;
        found = true;
    }
    file.close();

    if (nextBridgeId != nullptr)
    {
        if (previousId == 0xFFFFFFFFUL) return false;
        *nextBridgeId = previousId + 1UL;
    }
    return true;
}

bool SpoolMaterialBridgeStore::validateAll() const
{
    uint32_t batchSpoolIds[BridgeAuditBatchSize] = {};
    File outer = m_storage.open(Path, FILE_READ);
    if (!outer || outer.isDirectory())
    {
        if (outer) outer.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (outer.available())
    {
        uint8_t batchCount = 0U;
        while (outer.available() && batchCount < BridgeAuditBatchSize)
        {
            const String line = outer.readStringUntil('\n');
            if (line.length() == 0U) continue;

            SpoolMaterialBridge current;
            if (!parse(line, current) || current.bridgeId <= previousId)
            {
                outer.close();
                return false;
            }
            previousId = current.bridgeId;

            for (uint8_t i = 0U; i < batchCount; ++i)
            {
                if (batchSpoolIds[i] == current.spoolId)
                {
                    outer.close();
                    return false;
                }
            }
            batchSpoolIds[batchCount++] = current.spoolId;
        }

        if (batchCount == 0U) continue;

        const size_t suffixOffset = outer.position();
        if (suffixOffset > 0xFFFFFFFFUL)
        {
            outer.close();
            return false;
        }

        File suffix = m_storage.open(Path, FILE_READ);
        if (!suffix || suffix.isDirectory() ||
            !suffix.seek(static_cast<uint32_t>(suffixOffset)))
        {
            if (suffix) suffix.close();
            outer.close();
            return false;
        }

        uint32_t suffixPreviousId = previousId;
        while (suffix.available())
        {
            const String line = suffix.readStringUntil('\n');
            if (line.length() == 0U) continue;

            SpoolMaterialBridge current;
            if (!parse(line, current) || current.bridgeId <= suffixPreviousId)
            {
                suffix.close();
                outer.close();
                return false;
            }
            suffixPreviousId = current.bridgeId;

            for (uint8_t i = 0U; i < batchCount; ++i)
            {
                if (batchSpoolIds[i] == current.spoolId)
                {
                    suffix.close();
                    outer.close();
                    return false;
                }
            }
        }
        suffix.close();
    }

    outer.close();
    return true;
}

bool SpoolMaterialBridgeStore::parse(const String& line, SpoolMaterialBridge& bridge)
{
    bridge = SpoolMaterialBridge();
    uint32_t diameter = 0UL;
    if (!FlatJsonObjectValidator::valid(line) ||
        !findUnsigned(line, "bridge_id", bridge.bridgeId) ||
        !findUnsigned(line, "spool_id", bridge.spoolId) ||
        !findUnsigned(line, "warehouse_item_id", bridge.warehouseItemId) ||
        !findString(line, "wire_type", bridge.wireType) ||
        !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
        diameter == 0UL || diameter > 0xFFFFUL ||
        !findString(line, "linked_at", bridge.linkedAt))
    {
        return false;
    }
    bridge.diameterHundredthsMm = static_cast<uint16_t>(diameter);
    return bridge.valid();
}

bool SpoolMaterialBridgeStore::findUnsigned(const String& line,
                                            const char* key,
                                            uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool SpoolMaterialBridgeStore::findString(const String& line,
                                          const char* key,
                                          String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"')
            return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
        if (ch == '\\' || static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

String SpoolMaterialBridgeStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"') { escaped += '\\'; escaped += ch; }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
