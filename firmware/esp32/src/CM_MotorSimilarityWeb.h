#ifndef CM_MOTOR_SIMILARITY_WEB_H
#define CM_MOTOR_SIMILARITY_WEB_H

#include <WebServer.h>
#include "CM_MotorWindingVersionStore.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class MotorSimilarityWeb
{
public:
    MotorSimilarityWeb(WebServer& server, RepairRegistry& registry);
    void begin();

private:
    void handleLookup();
    void handleUpdate();
    void handleWindingRoleUpdate();

    WebServer& m_server;
    RepairRegistry& m_registry;
    MotorWindingVersionStore m_windingVersions;
    bool m_windingVersionsReady;
};
}

#endif
