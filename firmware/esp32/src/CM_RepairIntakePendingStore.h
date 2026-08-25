#ifndef CM_REPAIR_INTAKE_PENDING_STORE_H
#define CM_REPAIR_INTAKE_PENDING_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct RepairIntakePending
{
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;
    uint32_t sourceWindingVersionId;
    String receivedAt;
    String sourceKind;

    RepairIntakePending()
        : repairId(0UL), clientId(0UL), motorId(0UL), sourceWindingVersionId(0UL)
    {
    }

    bool valid() const
    {
        return repairId > 0UL && clientId > 0UL && motorId > 0UL &&
               receivedAt.length() >= 10U && sourceKind.length() > 0U;
    }
};

class RepairIntakePendingStore
{
public:
    static constexpr const char* Path = "/data/workshop/repair-intake.pending.json";
    static constexpr const char* TempPath = "/data/workshop/repair-intake.pending.tmp";

    explicit RepairIntakePendingStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool hasPending() const;
    bool save(const RepairIntakePending& pending);
    bool load(RepairIntakePending& pending, bool& found) const;
    bool clear();

private:
    bool ensureDirectory();
    bool recoverTemp();
    bool loadPath(const char* path, RepairIntakePending& pending) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
