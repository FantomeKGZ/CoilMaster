#include "CM_RepairFinalizationGuard.h"
#include "CM_RepairCosting.h"

namespace CM
{
RepairFinalizationCheck RepairFinalizationGuard::check(fs::FS& storage,
                                                       uint32_t repairId)
{
    if (repairId == 0UL)
        return RepairFinalizationCheck::IntegrityFailed;

    RepairCosting costing(storage);
    if (!costing.begin() || !costing.ready())
        return RepairFinalizationCheck::StorageUnavailable;

    RepairCostSummary summary;
    if (!costing.load(repairId, summary))
    {
        return costing.ready()
                   ? RepairFinalizationCheck::IntegrityFailed
                   : RepairFinalizationCheck::StorageUnavailable;
    }

    if (summary.repairId != repairId ||
        summary.wireCostMinor > 0xFFFFFFFFFFFFFFFFULL - summary.materialCostMinor)
    {
        return RepairFinalizationCheck::IntegrityFailed;
    }

    const uint64_t directCost = summary.wireCostMinor + summary.materialCostMinor;
    if (directCost > 0xFFFFFFFFFFFFFFFFULL - summary.labourCostMinor ||
        directCost + summary.labourCostMinor != summary.totalCostMinor)
    {
        return RepairFinalizationCheck::IntegrityFailed;
    }

    if (summary.copperWireCostMinor > 0xFFFFFFFFFFFFFFFFULL - summary.aluminiumWireCostMinor)
        return RepairFinalizationCheck::IntegrityFailed;
    const uint64_t knownWireCost = summary.copperWireCostMinor + summary.aluminiumWireCostMinor;
    if (knownWireCost > 0xFFFFFFFFFFFFFFFFULL - summary.unknownWireCostMinor)
        return RepairFinalizationCheck::IntegrityFailed;
    const uint64_t materialWireCost = knownWireCost + summary.unknownWireCostMinor;

    const uint32_t knownWireLines = static_cast<uint32_t>(summary.copperWireLineCount) +
                                    static_cast<uint32_t>(summary.aluminiumWireLineCount);
    const uint32_t materialWireLines = knownWireLines +
                                       static_cast<uint32_t>(summary.unknownWireLineCount);
    if (materialWireCost != summary.wireCostMinor ||
        materialWireLines != static_cast<uint32_t>(summary.wireLineCount))
    {
        return RepairFinalizationCheck::IntegrityFailed;
    }

    return RepairFinalizationCheck::Ready;
}
}
