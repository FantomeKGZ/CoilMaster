#ifndef CM_WAREHOUSE_STORE_H
#define CM_WAREHOUSE_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
constexpr uint8_t WarehouseMaxDiameters = 32U;

struct WireStockSummary
{
    uint16_t diameterHundredthsMm;
    uint32_t remainingGrams;
    uint8_t activeSpoolCount;
    uint32_t consumedMonthGrams;
    uint32_t consumedAllTimeGrams;

    WireStockSummary()
        : diameterHundredthsMm(0U), remainingGrams(0UL), activeSpoolCount(0U),
          consumedMonthGrams(0UL), consumedAllTimeGrams(0UL) {}
};

struct NewWireSpool
{
    uint16_t diameterHundredthsMm;
    uint32_t currentWeightGrams;
    String wireType;
    String manufacturer;
    String supplier;
    String batch;
    String storageLocation;
    String comment;

    NewWireSpool() : diameterHundredthsMm(0U), currentWeightGrams(0UL) {}
};

struct WarehousePrice
{
    uint32_t pricePerKgMinor;
    String currency;

    WarehousePrice() : pricePerKgMinor(0UL), currency("KGS") {}
};

class WarehouseStore
{
public:
    explicit WarehouseStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool loadSummary(const char* monthPrefix);
    bool addSpool(const NewWireSpool& spool, uint32_t& assignedSpoolId);
    bool setWarehousePrice(const WarehousePrice& price);
    bool loadWarehousePrice(WarehousePrice& price) const;

    // Appends active spool objects to an existing JSON array body. A zero
    // diameter returns every active spool; otherwise only the chosen diameter.
    bool appendActiveSpoolsJson(String& json,
                                uint16_t diameterHundredthsMm,
                                uint16_t& appendedCount) const;

    uint8_t summaryCount() const;
    bool summaryAt(uint8_t index, WireStockSummary& summary) const;
    uint32_t totalRemainingGrams() const;
    uint32_t totalConsumedMonthGrams() const;
    uint32_t totalConsumedAllTimeGrams() const;

private:
    static constexpr const char* SpoolsPath = "/data/warehouse/spools.ndjson";
    static constexpr const char* MovementsPath = "/data/warehouse/movements.ndjson";
    static constexpr const char* PricePath = "/data/warehouse/price.ndjson";

    bool ensureDirectories();
    void clearSummary();
    WireStockSummary* findOrCreate(uint16_t diameterHundredthsMm);
    bool readSpools();
    bool readMovements(const char* monthPrefix);
    bool nextSpoolId(uint32_t& id) const;

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    WireStockSummary m_summary[WarehouseMaxDiameters];
    uint8_t m_summaryCount;
    bool m_ready;
};
}

#endif // CM_WAREHOUSE_STORE_H
