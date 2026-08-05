#ifndef CM_CONDUCTOR_CALCULATOR_H
#define CM_CONDUCTOR_CALCULATOR_H

#include <Arduino.h>

namespace CM
{
enum class ConductorMaterial : uint8_t
{
    Copper = 0,
    Aluminium = 1
};

struct ConductorBundle
{
    ConductorMaterial material;
    uint16_t diameterHundredthsMm;
    uint8_t parallelStrands;

    ConductorBundle()
        : material(ConductorMaterial::Copper),
          diameterHundredthsMm(0U),
          parallelStrands(1U)
    {
    }
};

struct ConversionSettings
{
    // Target/source cross-section ratio in thousandths.
    // Separate values are kept for each direction and are configurable.
    uint16_t aluminiumToCopperPermille;
    uint16_t copperToAluminiumPermille;
    uint16_t allowedDeviationPermille;
    uint8_t maxTargetStrands;

    ConversionSettings()
        : aluminiumToCopperPermille(1000U),
          copperToAluminiumPermille(1000U),
          allowedDeviationPermille(50U),
          maxTargetStrands(6U)
    {
    }
};

struct WireCandidate
{
    uint16_t diameterHundredthsMm;
    uint32_t availableGrams;

    WireCandidate() : diameterHundredthsMm(0U), availableGrams(0UL) {}
};

struct ConversionOption
{
    bool valid;
    ConductorMaterial targetMaterial;
    uint16_t targetDiameterHundredthsMm;
    uint8_t targetParallelStrands;
    uint32_t targetAreaMicrometre2;
    int32_t deviationPermille;
    uint32_t availableGrams;

    ConversionOption()
        : valid(false),
          targetMaterial(ConductorMaterial::Copper),
          targetDiameterHundredthsMm(0U),
          targetParallelStrands(0U),
          targetAreaMicrometre2(0UL),
          deviationPermille(0L),
          availableGrams(0UL)
    {
    }
};

class ConductorCalculator
{
public:
    static uint32_t bundleAreaMicrometre2(const ConductorBundle& bundle);

    static uint32_t requiredTargetAreaMicrometre2(
        const ConductorBundle& source,
        ConductorMaterial targetMaterial,
        const ConversionSettings& settings);

    static bool findBestWarehouseOption(
        const ConductorBundle& source,
        ConductorMaterial targetMaterial,
        const ConversionSettings& settings,
        const WireCandidate* candidates,
        uint8_t candidateCount,
        ConversionOption& option);

private:
    static uint32_t singleWireAreaMicrometre2(uint16_t diameterHundredthsMm);
};
}

#endif // CM_CONDUCTOR_CALCULATOR_H
