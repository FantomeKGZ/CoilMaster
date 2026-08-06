#ifndef CM_REPAIR_COSTING_WEB_H
#define CM_REPAIR_COSTING_WEB_H

#include <Arduino.h>
#include <WebServer.h>
#include "CM_RepairCosting.h"

namespace CM
{
class RepairCostingWeb
{
public:
    RepairCostingWeb(WebServer& server, RepairCosting& costing);
    void begin();

private:
    void handleGet();
    void handlePricingHistory();
    void handleSavePricing();
    static bool parseUnsigned(WebServer& server,const char* name,uint32_t minValue,uint32_t maxValue,uint32_t& value);
    static bool parseUnsigned64(WebServer& server,const char* name,uint64_t& value);
    static void appendUInt64(String& target,uint64_t value);

    WebServer& m_server;
    RepairCosting& m_costing;
};
}

#endif
