#include "CM_ConductorCalculator.h"

namespace CM
{
namespace
{
constexpr uint32_t PiScaled = 3141593UL;
constexpr uint32_t PiScale = 1000000UL;

uint32_t absolute32(int32_t value)
{
    return value < 0 ? static_cast<uint32_t>(-value) : static_cast<uint32_t>(value);
}
}

uint32_t ConductorCalculator::singleWireAreaMicrometre2(uint16_t diameterHundredthsMm)
{
    if (diameterHundredthsMm == 0U)
    {
        return 0UL;
    }

    // Diameter in hundredths of a millimetre equals diameter * 10 in micrometres.
    const uint64_t diameterMicrometres = static_cast<uint64_t>(diameterHundredthsMm) * 10ULL;
    const uint64_t squared = diameterMicrometres * diameterMicrometres;
    const uint64_t area = squared * PiScaled / (4ULL * PiScale);
    return area > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : static_cast<uint32_t>(area);
}

uint32_t ConductorCalculator::bundleAreaMicrometre2(const ConductorBundle& bundle)
{
    if (bundle.parallelStrands == 0U)
    {
        return 0UL;
    }

    const uint64_t area = static_cast<uint64_t>(singleWireAreaMicrometre2(
                              bundle.diameterHundredthsMm)) *
                          bundle.parallelStrands;
    return area > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : static_cast<uint32_t>(area);
}

uint32_t ConductorCalculator::requiredTargetAreaMicrometre2(
    const ConductorBundle& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings)
{
    const uint32_t sourceArea = bundleAreaMicrometre2(source);
    if (sourceArea == 0UL || source.material == targetMaterial)
    {
        return sourceArea;
    }

    const uint16_t ratio = source.material == ConductorMaterial::Aluminium &&
                                   targetMaterial == ConductorMaterial::Copper
                               ? settings.aluminiumToCopperPermille
                               : settings.copperToAluminiumPermille;
    if (ratio == 0U)
    {
        return 0UL;
    }

    const uint64_t required =
        (static_cast<uint64_t>(sourceArea) * ratio + 500ULL) / 1000ULL;
    return required > 0xFFFFFFFFULL ? 0xFFFFFFFFUL
                                    : static_cast<uint32_t>(required);
}

bool ConductorCalculator::findBestWarehouseOption(
    const ConductorBundle& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings,
    const WireCandidate* candidates,
    uint8_t candidateCount,
    ConversionOption& option)
{
    option = ConversionOption();
    option.targetMaterial = targetMaterial;

    if (candidates == nullptr || candidateCount == 0U ||
        settings.maxTargetStrands == 0U)
    {
        return false;
    }

    const uint32_t requiredArea =
        requiredTargetAreaMicrometre2(source, targetMaterial, settings);
    if (requiredArea == 0UL)
    {
        return false;
    }

    uint32_t bestAbsoluteDeviation = 0xFFFFFFFFUL;
    for (uint8_t candidateIndex = 0U; candidateIndex < candidateCount;
         ++candidateIndex)
    {
        const WireCandidate& candidate = candidates[candidateIndex];
        const uint32_t oneWireArea =
            singleWireAreaMicrometre2(candidate.diameterHundredthsMm);
        if (oneWireArea == 0UL)
        {
            continue;
        }

        for (uint8_t strands = 1U; strands <= settings.maxTargetStrands;
             ++strands)
        {
            const uint64_t combined64 =
                static_cast<uint64_t>(oneWireArea) * strands;
            if (combined64 > 0xFFFFFFFFULL)
            {
                break;
            }

            const uint32_t combined = static_cast<uint32_t>(combined64);
            const int64_t difference = static_cast<int64_t>(combined) -
                                       static_cast<int64_t>(requiredArea);
            const int32_t deviation = static_cast<int32_t>(
                difference * 1000LL / static_cast<int64_t>(requiredArea));
            const uint32_t absoluteDeviation = absolute32(deviation);

            if (absoluteDeviation > settings.allowedDeviationPermille ||
                absoluteDeviation >= bestAbsoluteDeviation)
            {
                continue;
            }

            bestAbsoluteDeviation = absoluteDeviation;
            option.valid = true;
            option.targetDiameterHundredthsMm = candidate.diameterHundredthsMm;
            option.targetParallelStrands = strands;
            option.targetAreaMicrometre2 = combined;
            option.deviationPermille = deviation;
            option.availableGrams = candidate.availableGrams;
        }
    }

    return option.valid;
}
}
