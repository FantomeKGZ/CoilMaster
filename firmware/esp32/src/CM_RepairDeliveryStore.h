#ifndef CM_REPAIR_DELIVERY_STORE_H
#define CM_REPAIR_DELIVERY_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NewRepairDelivery
{
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;
    String deliveredAt;
    String comment;

    NewRepairDelivery() : repairId(0UL), clientId(0UL), motorId(0UL) {}
};

struct RepairDeliveryState
{
    uint32_t deliveryId;
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;
    String deliveredAt;
    String comment;

    RepairDeliveryState()
        : deliveryId(0UL), repairId(0UL), clientId(0UL), motorId(0UL) {}
};

class RepairDeliveryStore
{
public:
    static constexpr const char* Path = "/data/workshop/repair-deliveries.ndjson";

    explicit RepairDeliveryStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewRepairDelivery& delivery, uint32_t& deliveryId);
    bool resolveByRepair(uint32_t repairId,
                         RepairDeliveryState& state,
                         bool& found) const;

private:
    bool ensureDirectory();
    bool nextDeliveryId(uint32_t& deliveryId) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_REPAIR_DELIVERY_STORE_H
