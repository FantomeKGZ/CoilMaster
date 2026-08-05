#ifndef CM_STATIC_SITE_SERVER_H
#define CM_STATIC_SITE_SERVER_H

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

namespace CM
{
class StaticSiteServer
{
public:
    StaticSiteServer(WebServer& server, fs::FS& storage);

    // Registers explicit entry routes and installs the static-file fallback.
    // webRoot is the directory on microSD that contains index.html,
    // mobile/, desktop/ and sites/.
    void begin(const char* webRoot = "/web");

    bool serveCurrentRequest();
    bool storageReady() const;

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
    String m_webRoot;
    bool m_ready;
};
}

#endif // CM_STATIC_SITE_SERVER_H
