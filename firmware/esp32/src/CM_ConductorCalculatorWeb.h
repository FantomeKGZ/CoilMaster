#ifndef CM_CONDUCTOR_CALCULATOR_WEB_H
#define CM_CONDUCTOR_CALCULATOR_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_ConductorCalculator.h"
#include "CM_WarehouseStore.h"

namespace CM
{
class ConductorCalculatorWeb
{
public:
    ConductorCalculatorWeb(WebServer& server, WarehouseStore& warehouse);
    void begin();

private:
    void handleCalculate();
    static bool parseUnsignedArg(WebServer& server,
                                 const char* name,
                                 uint32_t minimum,
                                 uint32_t maximum,
                                 uint32_t& value);
    static bool parseMaterial(const String& value, ConductorMaterial& material);
    static const char* materialText(ConductorMaterial material);
    static const char* availabilityText(ConversionAvailability availability);

    WebServer& m_server;
    WarehouseStore& m_warehouse;
};
}

#endif // CM_CONDUCTOR_CALCULATOR_WEB_H
