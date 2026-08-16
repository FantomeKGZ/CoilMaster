#ifndef CM_STORAGE_DIAGNOSTICS_WEB_H
#define CM_STORAGE_DIAGNOSTICS_WEB_H

#include <FS.h>
#include <WebServer.h>

namespace CM
{
class StorageDiagnosticsWeb
{
public:
    StorageDiagnosticsWeb(WebServer& server, fs::FS& storage);

private:
    void handleGet();

    WebServer& m_server;
    fs::FS& m_storage;
};
}

#endif // CM_STORAGE_DIAGNOSTICS_WEB_H
