#ifndef CM_STATIC_SITE_SERVER_H
#define CM_STATIC_SITE_SERVER_H

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

#include "CM_WindingJournalQuery.h"
#include "CM_WindingJournalWeb.h"

namespace CM
{
class StaticSiteServer
{
public:
    StaticSiteServer(WebServer& server, fs::FS& storage);

    // Registers explicit entry routes and static-file handling. webRoot
    // contains index.html, mobile/, desktop/ and sites/ on microSD.
    void begin(const char* webRoot = "/web");

    bool serveCurrentRequest();
    bool storageReady() const;
    bool windingHistoryReady() const;

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
    WindingJournalQuery m_windingHistoryQuery;
    WindingJournalWeb m_windingHistoryWeb;
    String m_webRoot;
    bool m_ready;
};
}

#endif // CM_STATIC_SITE_SERVER_H
