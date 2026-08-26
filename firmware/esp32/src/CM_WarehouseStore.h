#ifndef CM_WAREHOUSE_STORE_H
#define CM_WAREHOUSE_STORE_H

#include <Arduino.h>
#include <FS.h>
#include "CM_ConductorCalculator.h"

namespace CM
{
constexpr uint8_t WarehouseMaxDiameters = 32U;
constexpr uint8_t WarehouseMaxListPageSize = 32U;

struct WireStockSummary{uint16_t diameterHundredthsMm;uint32_t remainingGrams;uint8_t activeSpoolCount;uint32_t consumedMonthGrams;uint32_t consumedAllTimeGrams;WireStockSummary():diameterHundredthsMm(0U),remainingGrams(0UL),activeSpoolCount(0U),consumedMonthGrams(0UL),consumedAllTimeGrams(0UL){}};
struct NewWireSpool{uint16_t diameterHundredthsMm;uint32_t currentWeightGrams;String wireType;String manufacturer;String supplier;String batch;String storageLocation;String comment;NewWireSpool():diameterHundredthsMm(0U),currentWeightGrams(0UL){}};
struct ActiveWireSpoolIdentity{uint32_t spoolId;uint16_t diameterHundredthsMm;uint32_t currentWeightGrams;String wireType;ActiveWireSpoolIdentity():spoolId(0UL),diameterHundredthsMm(0U),currentWeightGrams(0UL){}bool isValid()const{return spoolId!=0UL&&diameterHundredthsMm!=0U&&currentWeightGrams!=0UL&&(wireType=="CU"||wireType=="AL");}};
struct WarehousePrice{uint32_t pricePerKgMinor;String currency;WarehousePrice():pricePerKgMinor(0UL),currency("KGS") {}};
struct KgFirstWriteOff{uint32_t spoolId;uint32_t repairId;uint32_t sourceSessionId;uint32_t sourceRunId;uint16_t diameterHundredthsMm;uint32_t consumedGrams;String wireType;String timestamp;String comment;KgFirstWriteOff():spoolId(0UL),repairId(0UL),sourceSessionId(0UL),sourceRunId(0UL),diameterHundredthsMm(0U),consumedGrams(0UL){}};
struct WriteOffMaterialTotals{uint32_t copperGrams;uint32_t aluminiumGrams;uint32_t unknownGrams;uint64_t copperValueMinor;uint64_t aluminiumValueMinor;uint64_t unknownValueMinor;uint16_t copperCount;uint16_t aluminiumCount;uint16_t unknownCount;WriteOffMaterialTotals():copperGrams(0UL),aluminiumGrams(0UL),unknownGrams(0UL),copperValueMinor(0ULL),aluminiumValueMinor(0ULL),unknownValueMinor(0ULL),copperCount(0U),aluminiumCount(0U),unknownCount(0U){}};
struct KnownWireDiameter{uint16_t diameterHundredthsMm;uint32_t availableGrams;KnownWireDiameter():diameterHundredthsMm(0U),availableGrams(0UL){}};

class WarehouseStore
{
public:
    explicit WarehouseStore(fs::FS& storage);
    bool begin(); bool ready() const; bool loadSummary(const char* monthPrefix);
    fs::FS& storage() { return m_storage; }
    bool addSpool(const NewWireSpool& spool,uint32_t& assignedSpoolId);
    bool assignLegacySpoolMaterial(uint32_t spoolId,const String& wireType);
    bool loadActiveSpoolIdentity(uint32_t spoolId,ActiveWireSpoolIdentity& identity,bool& found) const;
    bool repairExists(uint32_t repairId,bool& found) const;
    bool confirmedWriteOffForSourceRun(uint32_t sourceSessionId,uint32_t sourceRunId,bool& found) const;
    bool prepareManagedRunWireWriteOff(const KgFirstWriteOff& operation,uint32_t& movementId);
    bool applyManagedRunWireSpoolWeight(uint32_t spoolId,uint32_t weightBeforeGrams,uint32_t weightAfterGrams,uint16_t diameterHundredthsMm,const String& wireType);
    bool confirmManagedRunWireWriteOff(uint32_t movementId,const KgFirstWriteOff& operation,uint32_t weightBeforeGrams,uint32_t weightAfterGrams);
    bool appendConfirmedWriteOffsPageJson(String& json,uint32_t repairId,uint32_t cursor,uint8_t limit,uint16_t& appendedCount,uint16_t& totalMatchingCount,uint32_t& nextCursor,bool& hasMore,uint32_t& totalConsumedGrams,uint64_t& totalConsumedValueMinor,WriteOffMaterialTotals& materialTotals) const;
    bool setWarehousePrice(const WarehousePrice& price);
    bool loadWarehousePrice(WarehousePrice& price) const;
    bool loadWarehousePrice(WarehousePrice& price,bool& configured) const;
    bool setConversionSettings(const ConversionSettings& settings);
    bool loadConversionSettings(ConversionSettings& settings) const;
    bool appendActiveSpoolsPageJson(String& json,uint16_t diameterHundredthsMm,const char* materialFilter,uint32_t cursor,uint8_t limit,uint16_t& appendedCount,uint16_t& totalMatchingCount,uint32_t& nextCursor,bool& hasMore) const;
    bool appendMaterialSummaryJson(String& json,const char* monthPrefix) const;
    bool loadKnownWireDiameters(const char* wireType,KnownWireDiameter* items,uint8_t capacity,uint8_t& count) const;
    uint8_t summaryCount() const; bool summaryAt(uint8_t index,WireStockSummary& summary) const;
    uint32_t totalRemainingGrams() const; uint32_t totalConsumedMonthGrams() const; uint32_t totalConsumedAllTimeGrams() const;

private:
    // Legacy journal-shape support is retained only so startup recovery can close
    // historical direct WRITE_OFF PENDING records. No direct mutation entrypoint remains.
    struct ConfirmedSpoolWriteOff{uint32_t spoolId;uint32_t repairId;uint32_t sourceSessionId;uint32_t sourceRunId;uint32_t weightBeforeGrams;uint32_t weightAfterGrams;String timestamp;String comment;ConfirmedSpoolWriteOff():spoolId(0UL),repairId(0UL),sourceSessionId(0UL),sourceRunId(0UL),weightBeforeGrams(0UL),weightAfterGrams(0UL){}};
    static constexpr const char* SpoolsPath="/data/warehouse/spools.ndjson";
    static constexpr const char* SpoolsTempPath="/data/warehouse/spools.tmp";
    static constexpr const char* SpoolsBackupPath="/data/warehouse/spools.bak";
    static constexpr const char* MovementsPath="/data/warehouse/movements.ndjson";
    static constexpr const char* PricePath="/data/warehouse/price.ndjson";
    static constexpr const char* RepairsPath="/data/workshop/repairs.ndjson";
    static constexpr const char* ConversionSettingsPath="/data/settings/conductor.json";
    static constexpr const char* ConversionSettingsTempPath="/data/settings/conductor.tmp";
    static constexpr const char* ConversionSettingsBackupPath="/data/settings/conductor.bak";
    bool ensureDirectories();
    bool recoverSpoolFileSwap();
    bool replaceSpoolsFileFromTemp();
    bool validateSpoolsFile(const char* path) const;
    bool recoverPendingWriteOff();
    bool recoverConversionSettingsFileSwap() const;
    bool loadConversionSettingsFromPath(const char* path,ConversionSettings& settings) const;
    void clearSummary(); WireStockSummary* findOrCreate(uint16_t diameterHundredthsMm);
    bool readSpools(); bool readMovements(const char* monthPrefix); bool nextSpoolId(uint32_t& id) const; bool nextMovementId(uint32_t& id) const;
    bool rewriteSpoolWeight(uint32_t spoolId,uint32_t expectedWeightGrams,uint32_t newWeightGrams,uint16_t& diameterHundredthsMm,String& wireType);
    bool appendWriteOffRecord(uint32_t movementId,const ConfirmedSpoolWriteOff& operation,uint16_t diameterHundredthsMm,uint32_t consumedGrams,const WarehousePrice& price,const char* status,const String& wireType);
    bool appendKgFirstWriteOffRecord(uint32_t movementId,const KgFirstWriteOff& operation,uint32_t weightBeforeGrams,uint32_t weightAfterGrams,const WarehousePrice& price,const char* status);
    static bool findUnsigned(const String& line,const char* key,uint32_t& value); static bool findString(const String& line,const char* key,String& value); static String jsonEscape(const String& value);
    fs::FS& m_storage; WireStockSummary m_summary[WarehouseMaxDiameters]; uint8_t m_summaryCount; bool m_ready;
};
}
#endif