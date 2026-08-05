#include "CM_ConductorCalculator.h"

namespace CM
{
namespace
{
constexpr uint32_t PiScaled = 3141593UL;
constexpr uint32_t PiScale = 1000000UL;
constexpr uint32_t PurchasePenalty = 1000000UL;
constexpr uint32_t StrandPenalty = 1000UL;

uint32_t absolute32(int32_t value)
{
    return value < 0 ? static_cast<uint32_t>(-value) : static_cast<uint32_t>(value);
}
}

uint32_t ConductorCalculator::singleWireAreaMicrometre2(uint16_t diameterHundredthsMm)
{
    if (diameterHundredthsMm == 0U) return 0UL;

    const uint64_t diameterMicrometres =
        static_cast<uint64_t>(diameterHundredthsMm) * 10ULL;
    const uint64_t squared = diameterMicrometres * diameterMicrometres;
    const uint64_t area = squared * PiScaled / (4ULL * PiScale);
    return area > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : static_cast<uint32_t>(area);
}

uint32_t ConductorCalculator::bundleAreaMicrometre2(const ConductorBundle& bundle)
{
    if (bundle.parallelStrands == 0U) return 0UL;

    const uint64_t area = static_cast<uint64_t>(
                              singleWireAreaMicrometre2(bundle.diameterHundredthsMm)) *
                          bundle.parallelStrands;
    return area > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : static_cast<uint32_t>(area);
}

uint32_t ConductorCalculator::requiredTargetAreaMicrometre2(
    const ConductorBundle& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings)
{
    const uint32_t sourceArea = bundleAreaMicrometre2(source);
    if (sourceArea == 0UL || source.material == targetMaterial) return sourceArea;

    const uint16_t ratio = source.material == ConductorMaterial::Aluminium &&
                                   targetMaterial == ConductorMaterial::Copper
                               ? settings.aluminiumToCopperPermille
                               : settings.copperToAluminiumPermille;
    if (ratio == 0U) return 0UL;

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
    ConversionOption options[MaxRecommendedConversionOptions];
    const uint8_t count = findRecommendedOptions(source,
                                                  targetMaterial,
                                                  settings,
                                                  candidates,
                                                  candidateCount,
                                                  options);
    if (count == 0U)
    {
        option = ConversionOption();
        option.targetMaterial = targetMaterial;
        return false;
    }

    option = options[0];
    return true;
}

uint8_t ConductorCalculator::findRecommendedOptions(
    const ConductorBundle& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings,
    const WireCandidate* candidates,
    uint8_t candidateCount,
    ConversionOption options[MaxRecommendedConversionOptions])
{
    for (uint8_t i = 0U; i < MaxRecommendedConversionOptions; ++i)
    {
        options[i] = ConversionOption();
        options[i].targetMaterial = targetMaterial;
    }

    if (candidates == nullptr || candidateCount == 0U ||
        settings.maxTargetStrands == 0U)
    {
        return 0U;
    }

    const uint32_t requiredArea =
        requiredTargetAreaMicrometre2(source, targetMaterial, settings);
    if (requiredArea == 0UL) return 0U;

    for (uint8_t candidateIndex = 0U; candidateIndex < candidateCount;
         ++candidateIndex)
    {
        const WireCandidate& wire = candidates[candidateIndex];
        if (!wire.catalogKnown || wire.diameterHundredthsMm == 0U) continue;

        const uint32_t oneWireArea =
            singleWireAreaMicrometre2(wire.diameterHundredthsMm);
        if (oneWireArea == 0UL) continue;

        for (uint8_t strands = 1U; strands <= settings.maxTargetStrands; ++strands)
        {
            const uint64_t combined64 = static_cast<uint64_t>(oneWireArea) * strands;
            if (combined64 > 0xFFFFFFFFULL) break;

            const uint32_t combined = static_cast<uint32_t>(combined64);
            const int64_t difference = static_cast<int64_t>(combined) -
                                       static_cast<int64_t>(requiredArea);
            const int32_t deviation = static_cast<int32_t>(
                difference * 1000LL / static_cast<int64_t>(requiredArea));
            if (absolute32(deviation) > settings.allowedDeviationPermille) continue;

            ConversionOption candidate;
            candidate.valid = true;
            candidate.targetMaterial = targetMaterial;
            candidate.availability = wire.availableGrams > 0UL
                                         ? ConversionAvailability::InStock
                                         : ConversionAvailability::PurchaseRequired;
            candidate.targetDiameterHundredthsMm = wire.diameterHundredthsMm;
            candidate.targetParallelStrands = strands;
            candidate.targetAreaMicrometre2 = combined;
            candidate.deviationPermille = deviation;
            candidate.availableGrams = wire.availableGrams;
            insertRanked(candidate, options);
        }
    }

    uint8_t count = 0U;
    while (count < MaxRecommendedConversionOptions && options[count].valid) ++count;
    return count;
}

uint32_t ConductorCalculator::optionScore(const ConversionOption& option)
{
    if (!option.valid) return 0xFFFFFFFFUL;

    uint64_t score = absolute32(option.deviationPermille);
    score += static_cast<uint64_t>(option.targetParallelStrands - 1U) * StrandPenalty;
    if (option.availability == ConversionAvailability::PurchaseRequired)
    {
        score += PurchasePenalty;
    }
    return score > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : static_cast<uint32_t>(score);
}

void ConductorCalculator::insertRanked(
    const ConversionOption& candidate,
    ConversionOption options[MaxRecommendedConversionOptions])
{
    const uint32_t candidateScore = optionScore(candidate);
    for (uint8_t index = 0U; index < MaxRecommendedConversionOptions; ++index)
    {
        if (!options[index].valid || candidateScore < optionScore(options[index]))
        {
            for (uint8_t move = MaxRecommendedConversionOptions - 1U;
                 move > index;
                 --move)
            {
                options[move] = options[move - 1U];
            }
            options[index] = candidate;
            return;
        }
    }
}
}
