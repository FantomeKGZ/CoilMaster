#ifndef CM_MATERIAL_USAGE_CORRECTION_WEB_H
#define CM_MATERIAL_USAGE_CORRECTION_WEB_H

#include <WebServer.h>

#include "CM_MaterialLedger.h"

namespace CM
{
class MaterialUsageCorrectionWeb
{
public:
    MaterialUsageCorrectionWeb(WebServer& server, MaterialLedger& ledger);
    void begin();

private:
    void handleGet();
    void handlePost();
    static bool parseUnsigned(WebServer& server,
                              const char* name,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t& value);
    static void appendUInt64(String& target, uint64_t value);

    WebServer& m_server;
    MaterialLedger& m_ledger;
};
}

#endif // CM_MATERIAL_USAGE_CORRECTION_WEB_H
