#ifndef CM_AUTONOMOUS_WINDING_WEB_H
#define CM_AUTONOMOUS_WINDING_WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "CM_AutonomousWindingArchive.h"
#include "CM_RepairRegistry.h"

namespace CM
{
class AutonomousWindingWeb
{
public:
    AutonomousWindingWeb(WebServer& server,
                         AutonomousWindingArchive& archive,
                         RepairRegistry& registry);

    void begin();

private:
    void handleList();
    void handleAssign();
    void handleCompletedWebJobsList();
    void handleCompletedWebJobAssign();
    static bool parseCanonicalUint32(const String& text, uint32_t& value);

    WebServer& m_server;
    AutonomousWindingArchive& m_archive;
    RepairRegistry& m_registry;
};
}

#endif
