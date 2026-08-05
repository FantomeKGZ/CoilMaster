#ifndef CM_MOTOR_SIMILARITY_WEB_H
#define CM_MOTOR_SIMILARITY_WEB_H

#include <WebServer.h>
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

    WebServer& m_server;
    RepairRegistry& m_registry;
};
}

#endif
