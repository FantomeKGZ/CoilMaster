#include "CM_RepairFinalizationGuard.h"
#include "CM_RepairCosting.h"
#include "CM_WindingJournalQuery.h"

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

    WindingJournalQuery history(storage);
    if (!history.begin() || !history.isReady())
        return RepairFinalizationCheck::StorageUnavailable;

    uint32_t cursor = 0UL;
    for (;;)
    {
        String page;
        page.reserve(4096U);
        uint16_t count = 0U;
        uint32_t nextCursor = cursor;
        bool hasMore = false;
        const WindingJournalQueryResult result =
            history.appendHistoryJson(0UL,
                                      repairId,
                                      cursor,
                                      100U,
                                      page,
                                      count,
                                      nextCursor,
                                      hasMore);
        if (result == WindingJournalQueryResult::StorageUnavailable)
            return RepairFinalizationCheck::StorageUnavailable;
        if (result != WindingJournalQueryResult::Ok)
            return RepairFinalizationCheck::IntegrityFailed;
        if (!hasMore) break;
        if (count == 0U || nextCursor <= cursor)
            return RepairFinalizationCheck::IntegrityFailed;
        cursor = nextCursor;
    }

    return RepairFinalizationCheck::Ready;
}
}
