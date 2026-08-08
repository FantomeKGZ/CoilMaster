#pragma once

#include <WebServer.h>
#include "CM_JobSpoolSelectionStore.h"

namespace CM
{
class JobSpoolSelectionWeb
{
public:
    JobSpoolSelectionWeb(WebServer& server, JobSpoolSelectionStore& store);
    void begin();

private:
    void handleGet();
    static bool parseCanonicalUint32(const String& source, uint32_t& value);

    WebServer& m_server;
    JobSpoolSelectionStore& m_store;
};
}
