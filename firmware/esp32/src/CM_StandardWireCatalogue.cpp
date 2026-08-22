#include "CM_StandardWireCatalogue.h"

namespace CM
{
namespace
{
// IEC 60317 preferred nominal R20 series, represented in CoilMaster's current
// 0.01 mm storage precision. Aluminium general requirements start at 0.25 mm.
constexpr uint16_t CopperR20Hundredths[] = {
    10U, 11U, 13U, 14U, 16U, 18U, 20U, 22U, 25U, 28U,
    32U, 36U, 40U, 45U, 50U, 56U, 63U, 71U, 80U, 90U,
    100U, 112U, 125U, 140U, 160U, 180U, 200U, 224U, 250U,
    280U, 315U, 355U, 400U, 450U, 500U
};

constexpr uint16_t AluminiumR20Hundredths[] = {
    25U, 28U, 32U, 36U, 40U, 45U, 50U, 56U, 63U, 71U,
    80U, 90U, 100U, 112U, 125U, 140U, 160U, 180U, 200U,
    224U, 250U, 280U, 315U, 355U, 400U, 450U, 500U
};

template <size_t N>
uint8_t copySeries(const uint16_t (&source)[N],
                   WireCandidate* candidates,
                   uint8_t capacity)
{
    if (candidates == nullptr || capacity == 0U) return 0U;
    const uint8_t count = static_cast<uint8_t>(
        N < static_cast<size_t>(capacity) ? N : static_cast<size_t>(capacity));
    for (uint8_t index = 0U; index < count; ++index)
    {
        candidates[index] = WireCandidate();
        candidates[index].diameterHundredthsMm = source[index];
        // Standard reference ranking must not be biased by warehouse history.
        // A non-zero synthetic value prevents the warehouse purchase penalty;
        // API serialization later reports real stock availability separately.
        candidates[index].availableGrams = 1UL;
        candidates[index].catalogKnown = true;
    }
    return count;
}
}

uint8_t StandardWireCatalogue::load(ConductorMaterial material,
                                    WireCandidate* candidates,
                                    uint8_t capacity)
{
    return material == ConductorMaterial::Aluminium
        ? copySeries(AluminiumR20Hundredths, candidates, capacity)
        : copySeries(CopperR20Hundredths, candidates, capacity);
}

const char* StandardWireCatalogue::basis()
{
    return "IEC_60317_R20_PROJECT_0_01_MM";
}
}
