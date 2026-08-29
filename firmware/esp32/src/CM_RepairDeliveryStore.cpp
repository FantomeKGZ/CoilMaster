#include "CM_RepairDeliveryStore.h"

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

RepairDeliveryStore::RepairDeliveryStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool RepairDeliveryStore::begin()
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
        uint32_t previousId = 0UL;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;
            uint32_t deliveryId = 0UL, repairId = 0UL, clientId = 0UL, motorId = 0UL;
            String deliveredAt;
            if (!FlatJsonObjectValidator::valid(line) ||
                !findUnsigned(line, "delivery_id", deliveryId) || deliveryId == 0UL ||
                deliveryId <= previousId ||
                !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
                !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
                !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
                !findString(line, "delivered_at", deliveredAt) ||
                deliveredAt.length() < 10U || deliveredAt.length() > 32U)
            {
                file.close();
                return false;
            }
            previousId = deliveryId;
        }
        file.close();
    }
    m_ready = true;
    return true;
}

bool RepairDeliveryStore::ready() const
{
    return m_ready;
}

bool RepairDeliveryStore::append(const NewRepairDelivery& delivery,
                                 uint32_t& deliveryId)
{
    bool alreadyExists = false;
    return append(delivery, deliveryId, alreadyExists) && !alreadyExists;
}

bool RepairDeliveryStore::append(const NewRepairDelivery& delivery,
                                 uint32_t& deliveryId,
                                 bool& alreadyExists)
{
    deliveryId = 0UL;
    alreadyExists = false;
    if (!ready() || delivery.repairId == 0UL || delivery.clientId == 0UL ||
        delivery.motorId == 0UL || delivery.deliveredAt.length() < 10U ||
        delivery.deliveredAt.length() > 32U || delivery.comment.length() > 500U)
    {
        return false;
    }

    if (!prepareAppend(delivery.repairId, deliveryId, alreadyExists)) return false;
    if (alreadyExists) return true;

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(700U);
    line = F("{\"delivery_id\":"); line += deliveryId;
    line += F(",\"repair_id\":"); line += delivery.repairId;
    line += F(",\"client_id\":"); line += delivery.clientId;
    line += F(",\"motor_id\":"); line += delivery.motorId;
    line += F(",\"delivered_at\":\""); line += jsonEscape(delivery.deliveredAt); line += '"';
    if (delivery.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(delivery.comment); line += '"';
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

bool RepairDeliveryStore::resolveByRepair(uint32_t repairId,
                                          RepairDeliveryState& state,
                                          bool& found) const
{
    state = RepairDeliveryState();
    found = false;
    if (!ready() || repairId == 0UL) return false;
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
        uint32_t deliveryId = 0UL, candidateRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "delivery_id", deliveryId) || deliveryId == 0UL ||
            deliveryId <= previousId ||
            !findUnsigned(line, "repair_id", candidateRepairId) || candidateRepairId == 0UL)
        {
            file.close();
            return false;
        }
        previousId = deliveryId;
        if (candidateRepairId != repairId) continue;
        if (found ||
            !findUnsigned(line, "client_id", state.clientId) || state.clientId == 0UL ||
            !findUnsigned(line, "motor_id", state.motorId) || state.motorId == 0UL ||
            !findString(line, "delivered_at", state.deliveredAt) ||
            state.deliveredAt.length() < 10U || state.deliveredAt.length() > 32U)
        {
            file.close();
            return false;
        }
        state.deliveryId = deliveryId;
        state.repairId = candidateRepairId;
        if (line.indexOf(F("\"comment\":")) >= 0 &&
            !findString(line, "comment", state.comment))
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();
    return true;
}

bool RepairDeliveryStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop")) return false;
    return true;
}

bool RepairDeliveryStore::prepareAppend(uint32_t repairId,
                                        uint32_t& deliveryId,
                                        bool& alreadyExists) const
{
    deliveryId = 1UL;
    alreadyExists = false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    bool found = false;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t candidateRepairId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "delivery_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "repair_id", candidateRepairId) || candidateRepairId == 0UL)
        {
            file.close();
            return false;
        }
        previousId = currentId;

        if (candidateRepairId != repairId) continue;

        uint32_t clientId = 0UL;
        uint32_t motorId = 0UL;
        String deliveredAt;
        String comment;
        if (found ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !findString(line, "delivered_at", deliveredAt) ||
            deliveredAt.length() < 10U || deliveredAt.length() > 32U ||
            (line.indexOf(F("\"comment\":")) >= 0 &&
             !findString(line, "comment", comment)))
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();

    if (found)
    {
        deliveryId = 0UL;
        alreadyExists = true;
        return true;
    }
    if (previousId == 0xFFFFFFFFUL) return false;
    deliveryId = previousId + 1UL;
    return true;
}

bool RepairDeliveryStore::findUnsigned(const String& line,
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

bool RepairDeliveryStore::findString(const String& line,
                                     const char* key,
                                     String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (escaped)
        {
            if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else if (ch == '\\' || ch == '"') value += ch;
            else return false;
            escaped = false;
            continue;
        }
        if (ch == '\\') { escaped = true; continue; }
        if (ch == '"') return true;
        value += ch;
    }
    return false;
}

String RepairDeliveryStore::jsonEscape(const String& value)
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
}
