#include "CM_RepairFinalizationGuard.h"
#include "CM_RepairCosting.h"
#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalTransitionAudit.h"

namespace CM
{
RepairFinalizationCheck RepairFinalizationGuard::check(fs::FS& storage,
                                                       uint32_t repairId)
{
    if (repairId == 0UL)
        return RepairFinalizationCheck::CostingIntegrityFailed;

    RepairCosting costing(storage);
    if (!costing.begin() || !costing.ready())
        return RepairFinalizationCheck::CostingStorageUnavailable;

    RepairCostSummary summary;
    if (!costing.load(repairId, summary))
    {
        return costing.ready()
                   ? RepairFinalizationCheck::CostingIntegrityFailed
                   : RepairFinalizationCheck::CostingStorageUnavailable;
    }

    if (summary.repairId != repairId ||
        summary.wireCostMinor > 0xFFFFFFFFFFFFFFFFULL - summary.materialCostMinor)
    {
        return RepairFinalizationCheck::CostingIntegrityFailed;
    }

    const uint64_t directCost = summary.wireCostMinor + summary.materialCostMinor;
    if (directCost > 0xFFFFFFFFFFFFFFFFULL - summary.labourCostMinor ||
        directCost + summary.labourCostMinor != summary.totalCostMinor)
    {
        return RepairFinalizationCheck::CostingIntegrityFailed;
    }

    if (summary.copperWireCostMinor > 0xFFFFFFFFFFFFFFFFULL - summary.aluminiumWireCostMinor)
        return RepairFinalizationCheck::CostingIntegrityFailed;
    const uint64_t knownWireCost = summary.copperWireCostMinor + summary.aluminiumWireCostMinor;
    if (knownWireCost > 0xFFFFFFFFFFFFFFFFULL - summary.unknownWireCostMinor)
        return RepairFinalizationCheck::CostingIntegrityFailed;
    const uint64_t materialWireCost = knownWireCost + summary.unknownWireCostMinor;

    const uint32_t knownWireLines = static_cast<uint32_t>(summary.copperWireLineCount) +
                                    static_cast<uint32_t>(summary.aluminiumWireLineCount);
    const uint32_t materialWireLines = knownWireLines +
                                       static_cast<uint32_t>(summary.unknownWireLineCount);
    if (materialWireCost != summary.wireCostMinor ||
        materialWireLines != static_cast<uint32_t>(summary.wireLineCount))
    {
        return RepairFinalizationCheck::CostingIntegrityFailed;
    }

    WindingJournalQuery history(storage);
    if (!history.begin() || !history.isReady())
        return RepairFinalizationCheck::WindingStorageUnavailable;

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
            return RepairFinalizationCheck::WindingStorageUnavailable;
        if (result != WindingJournalQueryResult::Ok)
            return RepairFinalizationCheck::WindingIntegrityFailed;
        if (!hasMore) break;
        if (count == 0U || nextCursor <= cursor)
            return RepairFinalizationCheck::WindingIntegrityFailed;
        cursor = nextCursor;
    }

    const WindingJournalTransitionAuditResult transitionAudit =
        WindingJournalTransitionAudit::validate(storage);
    if (transitionAudit == WindingJournalTransitionAuditResult::StorageUnavailable)
        return RepairFinalizationCheck::WindingStorageUnavailable;
    if (transitionAudit != WindingJournalTransitionAuditResult::Ok)
        return RepairFinalizationCheck::WindingIntegrityFailed;

    return RepairFinalizationCheck::Ready;
}
}
