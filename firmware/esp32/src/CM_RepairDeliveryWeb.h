#ifndef CM_REPAIR_DELIVERY_WEB_H
#define CM_REPAIR_DELIVERY_WEB_H

#include <WebServer.h>

#include "CM_RepairDeliveryStore.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class RepairDeliveryWeb
{
public:
    RepairDeliveryWeb(WebServer& server,
                      RepairRegistry& repairs,
                      RepairDeliveryStore& deliveries);

    void begin();

private:
    void handleGet();
    void handleCreate();
    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);
    static bool validTimestamp(const String& value);
    static String jsonEscape(const String& value);

    WebServer& m_server;
    RepairRegistry& m_repairs;
    RepairDeliveryStore& m_deliveries;
};
}

#endif // CM_REPAIR_DELIVERY_WEB_H
