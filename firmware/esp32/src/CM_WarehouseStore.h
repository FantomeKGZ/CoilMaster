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
        : diameterHundredthsMm(0U),
          remainingGrams(0UL),
          activeSpoolCount(0U),
          consumedMonthGrams(0UL),
          consumedAllTimeGrams(0UL)
    {
    }
};

class WarehouseStore
{
public:
    explicit WarehouseStore(fs::FS& storage);

    bool begin();
    bool ready() const;

    // Reads spool balances and confirmed repair write-offs from microSD.
    // monthPrefix must be YYYY-MM, for example "2026-08".
    bool loadSummary(const char* monthPrefix);

    uint8_t summaryCount() const;
    bool summaryAt(uint8_t index, WireStockSummary& summary) const;

    uint32_t totalRemainingGrams() const;
    uint32_t totalConsumedMonthGrams() const;
    uint32_t totalConsumedAllTimeGrams() const;

private:
    static constexpr const char* SpoolsPath = "/data/warehouse/spools.ndjson";
    static constexpr const char* MovementsPath = "/data/warehouse/movements.ndjson";

    bool ensureDirectories();
    void clearSummary();
    WireStockSummary* findOrCreate(uint16_t diameterHundredthsMm);
    bool readSpools();
    bool readMovements(const char* monthPrefix);

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);

    fs::FS& m_storage;
    WireStockSummary m_summary[WarehouseMaxDiameters];
    uint8_t m_summaryCount;
    bool m_ready;
};
}

#endif // CM_WAREHOUSE_STORE_H
