#ifndef CM_CASH_PAYMENT_WEB_H
#define CM_CASH_PAYMENT_WEB_H

#include <WebServer.h>

#include "CM_CashPaymentStore.h"
#include "CM_RepairCosting.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class CashPaymentWeb
{
public:
    CashPaymentWeb(WebServer& server,
                   RepairRegistry& repairs,
                   RepairCosting& costing,
                   CashPaymentStore& payments);
    void begin();

private:
    void handleList();
    void handleCreate();
    void handleBalance();
    bool sendRepairBalance(uint32_t repairId);
    bool sendClientBalance(uint32_t clientId);
    bool loadClientCharges(uint32_t clientId,
                           uint64_t& chargedMinor,
                           uint32_t& repairCount,
                           String& currency,
                           bool& currencySet);

    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);
    static bool parseUnsigned64(WebServer& server,
                                const char* name,
                                uint64_t minimum,
                                uint64_t& value);
    static bool parseRepairIds(const String& page,
                               uint16_t expectedCount,
                               uint32_t* ids,
                               uint8_t capacity,
                               uint8_t& count);
    static bool validTimestamp(const String& value);
    static bool validMethod(const String& value);
    static String jsonEscape(const String& value);
    static void appendUint64(String& target, uint64_t value);

    WebServer& m_server;
    RepairRegistry& m_repairs;
    RepairCosting& m_costing;
    CashPaymentStore& m_payments;
};
}

#endif // CM_CASH_PAYMENT_WEB_H
