#ifndef CM_STANDARD_WIRE_CATALOGUE_H
#define CM_STANDARD_WIRE_CATALOGUE_H

#include <Arduino.h>

#include "CM_ConductorCalculator.h"

namespace CM
{
constexpr uint8_t StandardWireMaxDiameters = 40U;

class StandardWireCatalogue
{
public:
    // Returns a read-only preferred nominal diameter catalogue derived from the
    // IEC 60317 R20 winding-wire series. CoilMaster stores diameter in 0.01 mm,
    // so sub-1 mm values that require finer precision are represented at the
    // nearest hundredth used by the current data model.
    static uint8_t load(ConductorMaterial material,
                        WireCandidate* candidates,
                        uint8_t capacity);
    static const char* basis();
};
}

#endif // CM_STANDARD_WIRE_CATALOGUE_H
