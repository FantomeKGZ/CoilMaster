#ifndef CM_WAREHOUSE_STORE_H
#define CM_WAREHOUSE_STORE_H

#include <Arduino.h>
#include <FS.h>
#include "CM_ConductorCalculator.h"

namespace CM
{
constexpr uint8_t WarehouseMaxDiameters = 32U;

struct WireStockSummary{uint16_t diameterHundredthsMm;uint32_t remainingGrams;uint8_t activeSpoolCount;uint32_t consumedMonthGrams;uint32_t consumedAllTimeGrams;WireStockSummary():diameterHundredthsMm(0U),remainingGrams(0UL),activeSpoolCount(0U),consumedMonthGrams(0UL),consumedAllTimeGrams(0UL){}};
struct NewWireSpool{uint16_t diameterHundredthsMm;uint32_t currentWeightGrams;String wireType;String manufacturer;String supplier;String batch;String storageLocation;String comment;NewWireSpool():diameterHundredthsMm(0U),currentWeightGrams(0UL){}};
struct WarehousePrice{uint32_t pricePerKgMinor;String currency;WarehousePrice():pricePerKgMinor(0UL),currency("KGS") {}};
struct ConfirmedSpoolWriteOff{uint32_t spoolId;uint32_t repairId;uint32_t weightBeforeGrams;uint32_t weightAfterGrams;String timestamp;String comment;ConfirmedSpoolWriteOff():spoolId(0UL),repairId(0UL),weightBeforeGrams(0UL),weightAfterGrams(0UL){}};
struct SpoolWriteOffResult{uint32_t movementId;uint16_t diameterHundredthsMm;uint32_t consumedGrams;uint32_t pricePerKgMinor;String currency;String wireType;SpoolWriteOffResult():movementId(0UL),diameterHundredthsMm(0U),consumedGrams(0UL),pricePerKgMinor(0UL),currency("KGS") {}};
struct KnownWireDiameter{uint16_t diameterHundredthsMm;uint32_t availableGrams;KnownWireDiameter():diameterHundredthsMm(0U),availableGrams(0UL){}};

class WarehouseStore
{
public:
    explicit WarehouseStore(fs::FS& storage);
    bool begin(); bool ready() const; bool loadSummary(const char* monthPrefix);
    fs::FS& storage() { return m_storage; }
    bool addSpool(const NewWireSpool& spool,uint32_t& assignedSpoolId);
    bool assignLegacySpoolMaterial(uint32_t spoolId,const String& wireType);
    bool confirmSpoolWriteOff(const ConfirmedSpoolWriteOff& operation,SpoolWriteOffResult& result);
    bool appendConfirmedWriteOffsJson(String& json,uint32_t repairId,uint16_t& appendedCount,uint32_t& totalConsumedGrams) const;
    bool setWarehousePrice(const WarehousePrice& price); bool loadWarehousePrice(WarehousePrice& price) const;
    bool setConversionSettings(const ConversionSettings& settings);
    bool loadConversionSettings(ConversionSettings& settings) const;
    bool appendActiveSpoolsJson(String& json,uint16_t diameterHundredthsMm,uint16_t& appendedCount) const;
    bool appendActiveSpoolsJson(String& json,uint16_t diameterHundredthsMm,const char* materialFilter,uint16_t& appendedCount) const;
    bool appendMaterialSummaryJson(String& json,const char* monthPrefix) const;
    uint8_t loadKnownWireDiameters(KnownWireDiameter* items,uint8_t capacity) const;
    uint8_t loadKnownWireDiameters(const char* wireType,KnownWireDiameter* items,uint8_t capacity) const;
    uint8_t summaryCount() const; bool summaryAt(uint8_t index,WireStockSummary& summary) const;
    uint32_t totalRemainingGrams() const; uint32_t totalConsumedMonthGrams() const; uint32_t totalConsumedAllTimeGrams() const;

private:
    static constexpr const char* SpoolsPath="/data/warehouse/spools.ndjson";
    static constexpr const char* SpoolsTempPath="/data/warehouse/spools.tmp";
    static constexpr const char* MovementsPath="/data/warehouse/movements.ndjson";
    static constexpr const char* PricePath="/data/warehouse/price.ndjson";
    static constexpr const char* ConversionSettingsPath="/data/settings/conductor.json";
    bool ensureDirectories(); void clearSummary(); WireStockSummary* findOrCreate(uint16_t diameterHundredthsMm);
    bool readSpools(); bool readMovements(const char* monthPrefix); bool nextSpoolId(uint32_t& id) const; bool nextMovementId(uint32_t& id) const;
    bool rewriteSpoolWeight(uint32_t spoolId,uint32_t expectedWeightGrams,uint32_t newWeightGrams,uint16_t& diameterHundredthsMm,String& wireType);
    bool appendWriteOffRecord(uint32_t movementId,const ConfirmedSpoolWriteOff& operation,uint16_t diameterHundredthsMm,uint32_t consumedGrams,const WarehousePrice& price,const char* status,const String& wireType);
    static bool findUnsigned(const String& line,const char* key,uint32_t& value); static bool findString(const String& line,const char* key,String& value); static String jsonEscape(const String& value);
    fs::FS& m_storage; WireStockSummary m_summary[WarehouseMaxDiameters]; uint8_t m_summaryCount; bool m_ready;
};
}
#endif
