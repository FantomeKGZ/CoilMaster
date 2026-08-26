#ifndef CM_CASH_PAYMENT_STORE_H
#define CM_CASH_PAYMENT_STORE_H

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

namespace CM
{
struct NewCashEvent
{
    uint32_t repairId = 0UL;
    uint32_t clientId = 0UL;
    String kind;
    String direction;
    uint64_t amountMinor = 0ULL;
    String currency;
    String occurredAt;
    String method;
    String comment;
    uint32_t correctsEventId = 0UL;
};

struct CashRepairTotals
{
    uint64_t paidMinor = 0ULL;
    uint32_t eventCount = 0UL;
    String currency;
    bool currencySet = false;
};

struct CashClientTotals
{
    uint64_t paidMinor = 0ULL;
    uint32_t eventCount = 0UL;
    String currency;
    bool currencySet = false;
};

class CashPaymentStore
{
public:
    static constexpr const char* Path = "/data/workshop/repair-payments.ndjson";
    static constexpr uint8_t MaxPageSize = 32U;

    explicit CashPaymentStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewCashEvent& event, uint32_t& eventId);
    bool eventExists(uint32_t eventId) const;
    bool eventBelongsToRepair(uint32_t eventId,
                              uint32_t repairId,
                              uint32_t clientId,
                              bool& found) const;
    bool totalsForRepair(uint32_t repairId, CashRepairTotals& totals) const;
    bool totalsForClient(uint32_t clientId, CashClientTotals& totals) const;
    bool appendRepairPageJson(String& json,
                              uint32_t repairId,
                              uint32_t cursor,
                              uint8_t limit,
                              uint16_t& count,
                              uint32_t& nextCursor,
                              bool& hasMore) const;
    bool appendClientPageJson(String& json,
                              uint32_t clientId,
                              uint32_t cursor,
                              uint8_t limit,
                              uint16_t& count,
                              uint32_t& nextCursor,
                              bool& hasMore) const;

private:
    bool ensureDirectory();
    bool nextEventId(uint32_t& eventId) const;
    bool validateJournal() const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_CASH_PAYMENT_STORE_H
