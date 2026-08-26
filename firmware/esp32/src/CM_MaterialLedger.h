#ifndef CM_MATERIAL_LEDGER_H
#define CM_MATERIAL_LEDGER_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
// Catalog stock is stored in thousandths of the declared unit. This legacy
// scale remains authoritative while Material Request adopts the same exact
// integer accounting model through explicit unit mapping.
enum class MaterialUnit : uint8_t
{
    Piece = 0,
    Gram = 1,
    Millilitre = 2,
    Metre = 3,
    SquareMetre = 4
};

struct NewMaterial
{
    String name;
    MaterialUnit unit;
    uint32_t stockQuantityMilli;
    uint32_t pricePerUnitMinor;
    String currency;
    String comment;
    // Optional pair. Legacy/generic records omit both. When present, both are
    // required and the material unit must be GRAM.
    bool hasWireMetadata;
    String wireType; // CU | AL
    uint16_t diameterHundredthsMm;

    NewMaterial()
        : unit(MaterialUnit::Piece), stockQuantityMilli(0UL),
          pricePerUnitMinor(0UL), currency("KGS"), hasWireMetadata(false),
          diameterHundredthsMm(0U) {}
};

struct MaterialItemState
{
    uint32_t materialId;
    MaterialUnit unit;
    uint32_t stockQuantityMilli;
    uint32_t pricePerUnitMinor;
    String currency;
    bool hasWireMetadata;
    String wireType; // CU | AL
    uint16_t diameterHundredthsMm;

    MaterialItemState()
        : materialId(0UL), unit(MaterialUnit::Piece), stockQuantityMilli(0UL),
          pricePerUnitMinor(0UL), currency("KGS"), hasWireMetadata(false),
          diameterHundredthsMm(0U) {}
};

struct MaterialAdjustment
{
    uint32_t materialId;
    uint32_t addQuantityMilli;
    uint32_t newPricePerUnitMinor;
    String currency;
    String timestamp;
    String comment;

    MaterialAdjustment()
        : materialId(0UL), addQuantityMilli(0UL),
          newPricePerUnitMinor(0UL), currency("KGS") {}
};

struct MaterialAdjustmentResult
{
    uint32_t adjustmentId;
    uint32_t stockQuantityMilli;
    uint32_t pricePerUnitMinor;
    String currency;

    MaterialAdjustmentResult()
        : adjustmentId(0UL), stockQuantityMilli(0UL),
          pricePerUnitMinor(0UL), currency("KGS") {}
};

struct RepairMaterialUsage
{
    uint32_t repairId;
    uint32_t materialId;
    uint32_t quantityMilli;
    String timestamp;
    String comment;

    RepairMaterialUsage()
        : repairId(0UL), materialId(0UL), quantityMilli(0UL) {}
};

struct RepairMaterialUsageResult
{
    uint32_t usageId;
    uint32_t remainingQuantityMilli;
    uint32_t unitPriceMinor;
    uint64_t lineCostMinor;
    String currency;

    RepairMaterialUsageResult()
        : usageId(0UL), remainingQuantityMilli(0UL),
          unitPriceMinor(0UL), lineCostMinor(0ULL), currency("KGS") {}
};

class MaterialLedger
{
public:
    static constexpr uint8_t MaxListPageSize = 32U;
    explicit MaterialLedger(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool repairExists(uint32_t repairId) const;
    bool repairExists(uint32_t repairId, bool& found) const;
    bool loadActiveMaterialCurrency(uint32_t materialId, String& currency) const;
    bool loadActiveMaterialCurrency(uint32_t materialId,
                                    String& currency,
                                    bool& found) const;
    bool loadActiveMaterialState(uint32_t materialId,
                                 MaterialItemState& state,
                                 bool& found) const;
    bool loadActiveMaterialState(uint32_t materialId,
                                 MaterialItemState& state) const;
    bool addMaterial(const NewMaterial& material, uint32_t& assignedMaterialId);
    bool adjustMaterial(const MaterialAdjustment& adjustment,
                        MaterialAdjustmentResult& result);
    bool appendMaterialsPageJson(String& json,
                                 uint32_t cursor,
                                 uint8_t limit,
                                 uint16_t& count,
                                 uint32_t& nextCursor,
                                 bool& hasMore) const;
    bool appendAdjustmentHistoryPageJson(String& json,
                                         uint32_t materialId,
                                         uint32_t cursor,
                                         uint8_t limit,
                                         uint16_t& count,
                                         uint32_t& nextCursor,
                                         bool& hasMore) const;
    bool appendUsageHistoryPageJson(String& json,
                                    uint32_t repairId,
                                    uint32_t materialId,
                                    uint32_t cursor,
                                    uint8_t limit,
                                    uint16_t& count,
                                    uint32_t& nextCursor,
                                    bool& hasMore) const;
    bool confirmUsage(const RepairMaterialUsage& usage,
                      RepairMaterialUsageResult& result);

private:
    static constexpr const char* MaterialsPath = "/data/materials/materials.ndjson";
    static constexpr const char* MaterialsTempPath = "/data/materials/materials.tmp";
    static constexpr const char* MaterialsBackupPath = "/data/materials/materials.bak";
    static constexpr const char* UsagePath = "/data/materials/usage.ndjson";
    static constexpr const char* UsagePendingPath = "/data/materials/usage.pending";
    static constexpr const char* AdjustmentsPath = "/data/materials/adjustments.ndjson";
    static constexpr const char* AdjustmentPendingPath = "/data/materials/adjustment.pending";
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";

    bool ensureDirectories();
    bool recoverMaterialFileSwap();
    bool replaceMaterialsFileFromTemp();
    bool validateMaterialsFile(const char* path) const;
    bool recoverPendingUsage();
    bool recoverPendingAdjustment();
    bool writePendingUsage(uint32_t usageId,
                           uint32_t materialId,
                           uint32_t stockBefore,
                           uint32_t stockAfter,
                           const String& usageLine);
    bool writePendingAdjustment(uint32_t adjustmentId,
                                uint32_t materialId,
                                uint32_t stockBefore,
                                uint32_t stockAfter,
                                uint32_t priceBefore,
                                uint32_t priceAfter,
                                const String& currencyBefore,
                                const String& currencyAfter,
                                const String& auditLine);
    bool usageExists(uint32_t usageId) const;
    bool adjustmentExists(uint32_t adjustmentId) const;
    bool readStockQuantity(uint32_t materialId, uint32_t& quantityMilli) const;
    bool readMaterialState(uint32_t materialId,
                           uint32_t& stockQuantityMilli,
                           uint32_t& pricePerUnitMinor,
                           String& currency) const;
    bool appendUsageLine(const String& line);
    bool appendAdjustmentLine(const String& line);
    bool nextId(const char* path, const char* key, uint32_t& id) const;
    bool rewriteQuantity(uint32_t materialId,
                         uint32_t consumeMilli,
                         uint32_t& stockBeforeMilli,
                         uint32_t& remainingMilli,
                         uint32_t& unitPriceMinor,
                         String& currency);
    bool restoreQuantity(uint32_t materialId, uint32_t quantityMilli);

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String replaceUnsigned(const String& line, const char* key, uint32_t value);
    static String replaceString(const String& line, const char* key, const String& value);
    static String jsonEscape(const String& value);
    static const char* unitText(MaterialUnit unit);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_MATERIAL_LEDGER_H
