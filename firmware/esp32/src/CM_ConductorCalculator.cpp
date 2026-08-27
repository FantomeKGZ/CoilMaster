#include "CM_ConductorCalculator.h"

namespace CM
{
namespace
{
constexpr uint32_t PiScaled = 3141593UL;
constexpr uint32_t PiScale = 1000000UL;
constexpr uint32_t PurchasePenalty = 1000000UL;
constexpr uint32_t StrandPenalty = 1000UL;
constexpr uint32_t MixedDiameterPenalty = 250UL;

uint32_t absolute32(int32_t value)
{
    return value < 0 ? static_cast<uint32_t>(-value) : static_cast<uint32_t>(value);
}
}

uint32_t ConductorCalculator::singleWireAreaMicrometre2(uint16_t diameterHundredthsMm)
{
    if (diameterHundredthsMm == 0U) return 0UL;
    const uint64_t diameterMicrometres = static_cast<uint64_t>(diameterHundredthsMm) * 10ULL;
    const uint64_t area = diameterMicrometres * diameterMicrometres * PiScaled /
                          (4ULL * PiScale);
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

uint32_t ConductorCalculator::sourceSetAreaMicrometre2(const SourceConductorSet& source)
{
    if (source.componentCount == 0U ||
        source.componentCount > MaxSourceConversionComponents) return 0UL;
    uint64_t total = 0ULL;
    for (uint8_t i = 0U; i < source.componentCount; ++i)
    {
        if (source.components[i].diameterHundredthsMm == 0U ||
            source.components[i].parallelStrands == 0U) return 0UL;
        total += static_cast<uint64_t>(singleWireAreaMicrometre2(
                     source.components[i].diameterHundredthsMm)) *
                 source.components[i].parallelStrands;
        if (total > 0xFFFFFFFFULL) return 0xFFFFFFFFUL;
    }
    return static_cast<uint32_t>(total);
}

uint32_t ConductorCalculator::requiredTargetAreaMicrometre2(
    uint32_t sourceArea,
    ConductorMaterial sourceMaterial,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings)
{
    if (sourceArea == 0UL || sourceMaterial == targetMaterial) return sourceArea;
    const uint16_t ratio = sourceMaterial == ConductorMaterial::Aluminium &&
                                   targetMaterial == ConductorMaterial::Copper
                               ? settings.aluminiumToCopperPermille
                               : settings.copperToAluminiumPermille;
    if (ratio == 0U) return 0UL;
    const uint64_t required =
        (static_cast<uint64_t>(sourceArea) * ratio + 500ULL) / 1000ULL;
    return required > 0xFFFFFFFFULL ? 0xFFFFFFFFUL
                                    : static_cast<uint32_t>(required);
}

uint32_t ConductorCalculator::requiredTargetAreaMicrometre2(
    const ConductorBundle& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings)
{
    return requiredTargetAreaMicrometre2(
        bundleAreaMicrometre2(source), source.material, targetMaterial, settings);
}

uint32_t ConductorCalculator::requiredTargetAreaMicrometre2(
    const SourceConductorSet& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings)
{
    return requiredTargetAreaMicrometre2(
        sourceSetAreaMicrometre2(source), source.material, targetMaterial, settings);
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
    const uint8_t count = findRecommendedOptions(source, targetMaterial, settings,
                                                  candidates, candidateCount, options);
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
    return findRecommendedOptionsForArea(
        requiredTargetAreaMicrometre2(source, targetMaterial, settings),
        targetMaterial, settings, candidates, candidateCount, options);
}

uint8_t ConductorCalculator::findRecommendedOptions(
    const SourceConductorSet& source,
    ConductorMaterial targetMaterial,
    const ConversionSettings& settings,
    const WireCandidate* candidates,
    uint8_t candidateCount,
    ConversionOption options[MaxRecommendedConversionOptions])
{
    return findRecommendedOptionsForArea(
        requiredTargetAreaMicrometre2(source, targetMaterial, settings),
        targetMaterial, settings, candidates, candidateCount, options);
}

uint8_t ConductorCalculator::findRecommendedOptionsForArea(
    uint32_t requiredArea,
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

    if (requiredArea == 0UL) return 0U;

    for (uint8_t firstIndex = 0U; firstIndex < candidateCount; ++firstIndex)
    {
        const WireCandidate& first = candidates[firstIndex];
        if (!first.catalogKnown || first.diameterHundredthsMm == 0U) continue;
        const uint32_t firstSingleArea =
            singleWireAreaMicrometre2(first.diameterHundredthsMm);
        if (firstSingleArea == 0UL) continue;

        for (uint8_t firstStrands = 1U;
             firstStrands <= settings.maxTargetStrands;
             ++firstStrands)
        {
            evaluateOption(first, firstSingleArea, firstStrands,
                           nullptr, 0UL, 0U, targetMaterial,
                           requiredArea, settings.allowedDeviationPermille, options);
        }

        if (!settings.allowMixedDiameters || settings.maxTargetStrands < 2U) continue;

        for (uint8_t secondIndex = static_cast<uint8_t>(firstIndex + 1U);
             secondIndex < candidateCount;
             ++secondIndex)
        {
            const WireCandidate& second = candidates[secondIndex];
            if (!second.catalogKnown || second.diameterHundredthsMm == 0U) continue;
            const uint32_t secondSingleArea =
                singleWireAreaMicrometre2(second.diameterHundredthsMm);
            if (secondSingleArea == 0UL) continue;

            for (uint8_t firstStrands = 1U;
                 firstStrands < settings.maxTargetStrands;
                 ++firstStrands)
            {
                const uint8_t maximumSecond = static_cast<uint8_t>(
                    settings.maxTargetStrands - firstStrands);
                for (uint8_t secondStrands = 1U;
                     secondStrands <= maximumSecond;
                     ++secondStrands)
                {
                    evaluateOption(first, firstSingleArea, firstStrands,
                                   &second, secondSingleArea, secondStrands,
                                   targetMaterial, requiredArea,
                                   settings.allowedDeviationPermille, options);
                }
            }
        }
    }

    uint8_t count = 0U;
    while (count < MaxRecommendedConversionOptions && options[count].valid) ++count;
    return count;
}

void ConductorCalculator::evaluateOption(
    const WireCandidate& first,
    uint32_t firstSingleArea,
    uint8_t firstStrands,
    const WireCandidate* second,
    uint32_t secondSingleArea,
    uint8_t secondStrands,
    ConductorMaterial targetMaterial,
    uint32_t requiredArea,
    uint16_t allowedDeviationPermille,
    ConversionOption options[MaxRecommendedConversionOptions])
{
    const uint64_t firstArea = static_cast<uint64_t>(firstSingleArea) * firstStrands;
    const uint64_t secondArea = second == nullptr
                                    ? 0ULL
                                    : static_cast<uint64_t>(secondSingleArea) *
                                          secondStrands;
    const uint64_t combined64 = firstArea + secondArea;
    if (combined64 == 0ULL || combined64 > 0xFFFFFFFFULL) return;

    const uint32_t combined = static_cast<uint32_t>(combined64);
    const int64_t difference = static_cast<int64_t>(combined) -
                               static_cast<int64_t>(requiredArea);
    const int32_t deviation = static_cast<int32_t>(
        difference * 1000LL / static_cast<int64_t>(requiredArea));
    if (absolute32(deviation) > allowedDeviationPermille) return;

    ConversionOption candidate;
    candidate.valid = true;
    candidate.targetMaterial = targetMaterial;
    candidate.componentCount = second == nullptr ? 1U : 2U;
    candidate.components[0].diameterHundredthsMm = first.diameterHundredthsMm;
    candidate.components[0].parallelStrands = firstStrands;
    candidate.components[0].availableGrams = first.availableGrams;
    if (second != nullptr)
    {
        candidate.components[1].diameterHundredthsMm = second->diameterHundredthsMm;
        candidate.components[1].parallelStrands = secondStrands;
        candidate.components[1].availableGrams = second->availableGrams;
    }

    candidate.targetDiameterHundredthsMm = first.diameterHundredthsMm;
    candidate.targetParallelStrands = static_cast<uint8_t>(firstStrands + secondStrands);
    candidate.targetAreaMicrometre2 = combined;
    candidate.deviationPermille = deviation;
    candidate.availableGrams = first.availableGrams;
    if (second != nullptr && second->availableGrams < candidate.availableGrams)
    {
        candidate.availableGrams = second->availableGrams;
    }

    const bool firstInStock = first.availableGrams > 0UL;
    const bool secondInStock = second == nullptr || second->availableGrams > 0UL;
    candidate.availability = firstInStock && secondInStock
                                 ? ConversionAvailability::InStock
                                 : ConversionAvailability::PurchaseRequired;
    candidate.rankingScore = optionScore(candidate);
    insertRanked(candidate, options);
}

uint32_t ConductorCalculator::optionScore(const ConversionOption& option)
{
    if (!option.valid) return 0xFFFFFFFFUL;
    uint64_t score = absolute32(option.deviationPermille);
    score += static_cast<uint64_t>(option.targetParallelStrands - 1U) * StrandPenalty;
    if (option.componentCount > 1U) score += MixedDiameterPenalty;
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
    const uint32_t candidateScore = candidate.rankingScore;
    for (uint8_t index = 0U; index < MaxRecommendedConversionOptions; ++index)
    {
        if (!options[index].valid || candidateScore < options[index].rankingScore)
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
