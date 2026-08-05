#ifndef CM_STATIC_SITE_SERVER_H
#define CM_STATIC_SITE_SERVER_H

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

#include "CM_WarehouseStore.h"
#include "CM_WarehouseWeb.h"

namespace CM
{
class StaticSiteServer
{
public:
    StaticSiteServer(WebServer& server, fs::FS& storage);

    // Registers explicit entry routes, the warehouse summary API and the
    // static-file fallback. webRoot contains index.html, mobile/, desktop/
    // and sites/ on microSD.
    void begin(const char* webRoot = "/web");

    bool serveCurrentRequest();
    bool storageReady() const;
    bool warehouseReady() const;

private:
    static constexpr size_t MaxPathLength = 192U;

    bool serveUri(const String& uri);
    bool resolvePath(const String& uri, String& resolvedPath) const;
    bool tryVariantFallback(const String& requestedPath, String& resolvedPath) const;
    bool streamFile(const String& path);
    bool isSafeUri(const String& uri) const;
    const char* contentTypeFor(const String& path) const;
    void redirect(const char* location);

    WebServer& m_server;
    fs::FS& m_storage;
    WarehouseStore m_warehouse;
    WarehouseWeb m_warehouseWeb;
    String m_webRoot;
    bool m_ready;
};
}

#endif // CM_STATIC_SITE_SERVER_H
