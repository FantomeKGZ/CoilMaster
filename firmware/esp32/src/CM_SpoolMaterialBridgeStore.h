#ifndef CM_SPOOL_MATERIAL_BRIDGE_STORE_H
#define CM_SPOOL_MATERIAL_BRIDGE_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct SpoolMaterialBridge
{
    uint32_t bridgeId;
    uint32_t spoolId;
    uint32_t warehouseItemId;
    String wireType; // CU | AL
    uint16_t diameterHundredthsMm;
    String linkedAt;

    SpoolMaterialBridge()
        : bridgeId(0UL), spoolId(0UL), warehouseItemId(0UL),
          diameterHundredthsMm(0U)
    {
    }

    bool valid() const
    {
        return bridgeId != 0UL && spoolId != 0UL && warehouseItemId != 0UL &&
               (wireType == "CU" || wireType == "AL") &&
               diameterHundredthsMm != 0U &&
               linkedAt.length() >= 10U && linkedAt.length() <= 32U;
    }
};

struct NewSpoolMaterialBridge
{
    uint32_t spoolId;
    uint32_t warehouseItemId;
    String wireType;
    uint16_t diameterHundredthsMm;
    String linkedAt;

    NewSpoolMaterialBridge()
        : spoolId(0UL), warehouseItemId(0UL), diameterHundredthsMm(0U)
    {
    }
};

class SpoolMaterialBridgeStore
{
public:
    static constexpr const char* Path =
        "/data/warehouse/spool-material-bridges.ndjson";

    explicit SpoolMaterialBridgeStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewSpoolMaterialBridge& bridge, uint32_t& bridgeId);
    bool loadBySpool(uint32_t spoolId, SpoolMaterialBridge& bridge, bool& found) const;
    bool validateAll() const;

private:
    bool ensureDirectory();
    bool nextBridgeId(uint32_t& bridgeId) const;
    static bool validNew(const NewSpoolMaterialBridge& bridge);
    static bool parse(const String& line, SpoolMaterialBridge& bridge);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_SPOOL_MATERIAL_BRIDGE_STORE_H
