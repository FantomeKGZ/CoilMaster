#include "CM_MaterialRequestMovementStore.h"

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

void appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%llu",
             static_cast<unsigned long long>(value));
    target += buffer;
}
}

MaterialRequestMovementStore::MaterialRequestMovementStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MaterialRequestMovementStore::begin()
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

bool MaterialRequestMovementStore::ready() const
{
    return m_ready;
}

bool MaterialRequestMovementStore::append(const NewMaterialRequestMovement& movement,
                                          uint32_t& movementId)
{
    movementId = 0UL;
    if (!ready() || !validMovement(movement) || !nextMovementId(movementId))
        return false;

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(940U);
    line = F("{\"movement_id\":"); line += movementId;
    line += F(",\"material_request_id\":"); line += movement.materialRequestId;
    line += F(",\"repair_id\":"); line += movement.repairId;
    line += F(",\"warehouse_item_id\":"); line += movement.warehouseItemId;
    line += F(",\"movement_kind\":\""); line += movement.movementKind;
    line += F("\",\"source_kind\":\""); line += movement.sourceKind;
    if (movement.movementKind == "CORRECTION")
    {
        line += F("\",\"correction_direction\":\"");
        line += movement.correctionDirection;
    }
    line += F("\",\"quantity_milli_units\":"); line += movement.quantityMilliUnits;
    line += F(",\"unit\":\""); line += movement.unit;
    line += F("\",\"unit_cost_minor\":"); appendUint64(line, movement.unitCostMinor);
    line += F(",\"cost_amount_minor\":"); appendUint64(line, movement.costAmountMinor);
    line += F(",\"currency\":\""); line += movement.currency;
    line += F("\",\"created_at\":\""); line += jsonEscape(movement.createdAt);
    line += '"';

    if (movement.sourceKind == "RUN_WIRE")
    {
        line += F(",\"source_session_id\":"); line += movement.sourceSessionId;
        line += F(",\"source_run_id\":"); line += movement.sourceRunId;
        line += F(",\"material_class\":\""); line += movement.materialClass;
        line += F("\",\"wire_diameter_hundredths_mm\":");
        line += movement.wireDiameterHundredthsMm;
    }
    if (movement.comment.length() > 0U)
    {
        line += F(",\"comment\":\"");
        line += jsonEscape(movement.comment);
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

bool MaterialRequestMovementStore::appendRequestPageJson(String& json,
                                                         uint32_t materialRequestId,
                                                         uint32_t cursor,
                                                         uint8_t limit,
                                                         uint16_t& count,
                                                         uint32_t& nextCursor,
                                                         bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || materialRequestId == 0UL || limit == 0U || limit > MaxPageSize)
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
        uint32_t movementId = 0UL;
        uint32_t requestId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", movementId) || movementId == 0UL ||
            movementId <= previous ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL)
        {
            file.close();
            return false;
        }
        previous = movementId;
        if (movementId <= cursor || requestId != materialRequestId) continue;
        if (count >= limit)
        {
            hasMore = true;
            break;
        }
        if (count > 0U) json += ',';
        json += line;
        ++count;
        nextCursor = movementId;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
    return true;
}

bool MaterialRequestMovementStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool MaterialRequestMovementStore::nextMovementId(uint32_t& movementId) const
{
    movementId = 1UL;
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
            !findUnsigned(line, "movement_id", current) || current == 0UL ||
            current <= previous)
        {
            file.close();
            return false;
        }
        previous = current;
    }
    file.close();
    if (previous == 0xFFFFFFFFUL) return false;
    movementId = previous + 1UL;
    return true;
}

bool MaterialRequestMovementStore::validMovement(
    const NewMaterialRequestMovement& movement)
{
    if (movement.materialRequestId == 0UL || movement.repairId == 0UL ||
        movement.warehouseItemId == 0UL || movement.quantityMilliUnits == 0UL ||
        !validUnit(movement.unit) || !validCurrency(movement.currency) ||
        movement.createdAt.length() < 10U || movement.createdAt.length() > 32U ||
        movement.comment.length() > 500U)
    {
        return false;
    }

    if (movement.movementKind != "ISSUE" && movement.movementKind != "RETURN" &&
        movement.movementKind != "CORRECTION")
    {
        return false;
    }
    if (movement.movementKind == "CORRECTION")
    {
        if (movement.correctionDirection != "ADD" &&
            movement.correctionDirection != "REMOVE")
        {
            return false;
        }
    }
    else if (movement.correctionDirection.length() != 0U)
    {
        return false;
    }

    if (movement.unit == "PCS" && (movement.quantityMilliUnits % 1000UL) != 0UL)
        return false;

    if (movement.sourceKind == "RUN_WIRE")
    {
        return movement.movementKind == "ISSUE" &&
               movement.sourceSessionId > 0UL && movement.sourceRunId > 0UL &&
               (movement.materialClass == "CU" || movement.materialClass == "AL") &&
               movement.wireDiameterHundredthsMm > 0U &&
               movement.wireDiameterHundredthsMm <= 500U &&
               movement.unit == "KG";
    }

    if (movement.sourceKind != "MANUAL_MATERIAL") return false;
    return movement.sourceSessionId == 0UL && movement.sourceRunId == 0UL &&
           movement.materialClass.length() == 0U &&
           movement.wireDiameterHundredthsMm == 0U;
}

bool MaterialRequestMovementStore::validUnit(const String& unit)
{
    return unit == "KG" || unit == "L" || unit == "PCS" || unit == "M" ||
           unit == "M2";
}

bool MaterialRequestMovementStore::validCurrency(const String& currency)
{
    if (currency.length() != 3U) return false;
    for (size_t i = 0U; i < currency.length(); ++i)
    {
        if (currency[i] < 'A' || currency[i] > 'Z') return false;
    }
    return true;
}

bool MaterialRequestMovementStore::findUnsigned(const String& line,
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

String MaterialRequestMovementStore::jsonEscape(const String& value)
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
