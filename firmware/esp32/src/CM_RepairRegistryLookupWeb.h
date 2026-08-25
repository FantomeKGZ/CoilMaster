#ifndef CM_REPAIR_REGISTRY_LOOKUP_WEB_H
#define CM_REPAIR_REGISTRY_LOOKUP_WEB_H

#include <WebServer.h>

#include "CM_MotorWindingVersionStore.h"
#include "CM_RepairAsReceivedSnapshotStore.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class RepairRegistryLookupWeb
{
public:
    RepairRegistryLookupWeb(WebServer& server, RepairRegistry& registry);
    void begin();

private:
    void handleClient();
    void handleMotor();
    void handleRepair();
    void handleMotorRepairs();
    void handleMotorWindingLatest();
    void handleMotorWindingVersions();
    void handleRepairAsReceived();
    void handleClientSearch();
    void handleRepairSearch();
    bool parseId(const char* name, uint32_t& value) const;
    bool parseOptionalUnsigned(const char* name,
                               uint32_t minimum,
                               uint32_t maximum,
                               uint32_t& value,
                               bool& present) const;

    WebServer& m_server;
    RepairRegistry& m_registry;
    MotorWindingVersionStore m_windingVersions;
    RepairAsReceivedSnapshotStore m_asReceivedSnapshots;
    bool m_windingVersionsReady;
    bool m_asReceivedSnapshotsReady;
};
}

#endif
