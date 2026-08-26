#include "CM_SpoolMaterialBridgeIntegrityAudit.h"

#include <Arduino.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_SpoolMaterialBridgeStore.h"

namespace CM
{
namespace
{
constexpr uint8_t ReferenceBatchSize = 24U;
constexpr const char* SpoolsPath = "/data/warehouse/spools.ndjson";
constexpr const char* MaterialsPath = "/data/materials/materials.ndjson";

struct BridgeReference
{
    uint32_t spoolId;
    uint32_t warehouseItemId;
    uint16_t diameterHundredthsMm;
    String wireType;
    uint8_t spoolMatches;
    uint8_t materialMatches;

    BridgeReference()
        : spoolId(0UL), warehouseItemId(0UL), diameterHundredthsMm(0U),
          spoolMatches(0U), materialMatches(0U)
    {
    }
};

bool findUnsigned(const String& line, const char* key, uint32_t& value)
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
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool findString(const String& line, const char* key, String& value)
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
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool resolveSpoolReferences(fs::FS& storage,
                            BridgeReference* references,
                            uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(SpoolsPath)) return false;
    File file = storage.open(SpoolsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t spoolId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t i = 0U; i < count; ++i)
        {
            BridgeReference& reference = references[i];
            if (reference.spoolId != spoolId) continue;
            if (reference.spoolMatches != 0U)
            {
                file.close();
                return false;
            }
            uint32_t diameter = 0UL;
            String wireType;
            if (!findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                diameter != reference.diameterHundredthsMm ||
                !findString(line, "wire_type", wireType) ||
                wireType != reference.wireType)
            {
                file.close();
                return false;
            }
            reference.spoolMatches = 1U;
        }
    }
    file.close();

    for (uint8_t i = 0U; i < count; ++i)
    {
        if (references[i].spoolMatches != 1U) return false;
    }
    return true;
}

bool resolveMaterialReferences(fs::FS& storage,
                               BridgeReference* references,
                               uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(MaterialsPath)) return false;
    File file = storage.open(MaterialsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t materialId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_id", materialId) || materialId == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t i = 0U; i < count; ++i)
        {
            BridgeReference& reference = references[i];
            if (reference.warehouseItemId != materialId) continue;
            if (reference.materialMatches != 0U)
            {
                file.close();
                return false;
            }
            String unit;
            if (!findString(line, "unit", unit) || unit != "GRAM")
            {
                file.close();
                return false;
            }
            reference.materialMatches = 1U;
        }
    }
    file.close();

    for (uint8_t i = 0U; i < count; ++i)
    {
        if (references[i].materialMatches != 1U) return false;
    }
    return true;
}

bool validateBatch(fs::FS& storage,
                   BridgeReference* references,
                   uint8_t count)
{
    return resolveSpoolReferences(storage, references, count) &&
           resolveMaterialReferences(storage, references, count);
}
}

bool SpoolMaterialBridgeIntegrityAudit::check(fs::FS& storage,
                                              uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(SpoolMaterialBridgeStore::Path)) return true;

    SpoolMaterialBridgeStore store(storage);
    if (!store.validateAll()) return false;

    File file = storage.open(SpoolMaterialBridgeStore::Path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    BridgeReference references[ReferenceBatchSize];
    uint8_t count = 0U;
    uint32_t previousBridgeId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t bridgeId = 0UL, spoolId = 0UL, itemId = 0UL, diameter = 0UL;
        String wireType;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "bridge_id", bridgeId) || bridgeId == 0UL ||
            bridgeId <= previousBridgeId ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            !findUnsigned(line, "warehouse_item_id", itemId) || itemId == 0UL ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findString(line, "wire_type", wireType) ||
            (wireType != "CU" && wireType != "AL"))
        {
            file.close();
            return false;
        }
        previousBridgeId = bridgeId;
        if (recordCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++recordCount;

        BridgeReference& reference = references[count++];
        reference.spoolId = spoolId;
        reference.warehouseItemId = itemId;
        reference.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        reference.wireType = wireType;
        reference.spoolMatches = 0U;
        reference.materialMatches = 0U;

        if (count == ReferenceBatchSize)
        {
            if (!validateBatch(storage, references, count))
            {
                file.close();
                return false;
            }
            count = 0U;
        }
    }

    if (count > 0U && !validateBatch(storage, references, count))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}
}
