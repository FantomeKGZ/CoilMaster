#ifndef CM_RUN_WIRE_ISSUE_PENDING_STORE_H
#define CM_RUN_WIRE_ISSUE_PENDING_STORE_H

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

namespace CM
{
struct RunWireIssuePending
{
    String transactionRef;
    uint32_t materialRequestId;
    uint32_t repairId;
    uint32_t warehouseItemId;
    uint32_t sourceSessionId;
    uint32_t sourceRunId;
    uint32_t spoolId;
    uint32_t consumedGrams;
    uint32_t spoolWeightBeforeGrams;
    uint32_t spoolWeightAfterGrams;
    uint32_t ledgerQuantityMilli;
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;
    String materialClass;
    uint16_t wireDiameterHundredthsMm;
    String createdAt;
    String comment;

    RunWireIssuePending()
        : materialRequestId(0UL), repairId(0UL), warehouseItemId(0UL),
          sourceSessionId(0UL), sourceRunId(0UL), spoolId(0UL),
          consumedGrams(0UL), spoolWeightBeforeGrams(0UL),
          spoolWeightAfterGrams(0UL), ledgerQuantityMilli(0UL),
          unitCostMinor(0ULL), costAmountMinor(0ULL),
          wireDiameterHundredthsMm(0U)
    {
    }

    bool valid() const;
};

class RunWireIssuePendingStore
{
public:
    static constexpr const char* Path =
        "/data/workshop/run-wire-issue.pending.json";
    static constexpr const char* TempPath =
        "/data/workshop/run-wire-issue.pending.tmp";

    explicit RunWireIssuePendingStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool hasPending() const;
    bool save(const RunWireIssuePending& pending);
    bool load(RunWireIssuePending& pending, bool& found) const;
    bool clear();

private:
    bool ensureDirectory();
    bool recoverTemp();
    bool loadPath(const char* path, RunWireIssuePending& pending) const;

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);
    static void appendUint64(String& target, uint64_t value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_RUN_WIRE_ISSUE_PENDING_STORE_H
