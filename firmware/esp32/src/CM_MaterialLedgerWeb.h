#ifndef CM_MATERIAL_LEDGER_WEB_H
#define CM_MATERIAL_LEDGER_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_MaterialLedger.h"

namespace CM
{
class MaterialLedgerWeb
{
public:
    MaterialLedgerWeb(WebServer& server, MaterialLedger& ledger);
    void begin();

private:
    void handleList();
    void handleCreate();
    void handleAdjust();
    void handleAdjustmentHistory();
    void handleUsage();
    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);
    static bool parseUnit(const String& source, MaterialUnit& unit);

    WebServer& m_server;
    MaterialLedger& m_ledger;
};
}

#endif // CM_MATERIAL_LEDGER_WEB_H
