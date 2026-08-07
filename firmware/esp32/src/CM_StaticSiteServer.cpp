#include "CM_StaticSiteServer.h"

namespace CM
{
StaticSiteServer::StaticSiteServer(WebServer& server, fs::FS& storage)
    : m_server(server),
      m_storage(storage),
      m_windingHistoryQuery(storage),
      m_windingHistoryWeb(server, m_windingHistoryQuery),
      m_webRoot("/web"),
      m_ready(false)
{
}

void StaticSiteServer::begin(const char* webRoot)
{
    m_webRoot = webRoot != nullptr && webRoot[0] != '\0' ? webRoot : "/web";
    if (!m_webRoot.startsWith("/"))
    {
        m_webRoot = '/' + m_webRoot;
    }
    while (m_webRoot.length() > 1U && m_webRoot.endsWith("/"))
    {
        m_webRoot.remove(m_webRoot.length() - 1U);
    }

    m_ready = m_storage.exists(m_webRoot);

    // The history API is registered through the same web initialization point,
    // but remains a separate read-only module over winding events on microSD.
    m_windingHistoryQuery.begin();
    m_windingHistoryWeb.begin();

    m_server.on("/mobile", HTTP_GET, [this]() { redirect("/mobile/"); });
    m_server.on("/desktop", HTTP_GET, [this]() { redirect("/desktop/"); });

    m_server.on("/sites/reference", HTTP_GET, [this]()
    {
        // The browser remembers the selected interface. This small redirector
        // lets JavaScript choose the matching reference-site variant.
        static const char Page[] PROGMEM =
            "<!doctype html><meta charset=\"utf-8\"><script>"
            "const v=localStorage.getItem('cm-ui-version')==='desktop'?'desktop':'mobile';"
            "location.replace('/sites/reference/'+v+'/');"
            "</script><noscript><a href=\"/sites/reference/mobile/\">Открыть справочник</a></noscript>";
        m_server.send_P(200, "text/html; charset=utf-8", Page);
    });
    m_server.on("/sites/reference/", HTTP_GET, [this]()
    {
        static const char Page[] PROGMEM =
            "<!doctype html><meta charset=\"utf-8\"><script>"
            "const v=localStorage.getItem('cm-ui-version')==='desktop'?'desktop':'mobile';"
            "location.replace('/sites/reference/'+v+'/');"
            "</script><noscript><a href=\"/sites/reference/mobile/\">Открыть справочник</a></noscript>";
        m_server.send_P(200, "text/html; charset=utf-8", Page);
    });
}

bool StaticSiteServer::serveCurrentRequest()
{
    return serveUri(m_server.uri());
}

bool StaticSiteServer::storageReady() const
{
    if (!m_ready) return false;
    File root = m_storage.open(m_webRoot, FILE_READ);
    if (!root) return false;
    const bool ready = root.isDirectory();
    root.close();
    return ready;
}

bool StaticSiteServer::windingHistoryReady() const
{
    return m_windingHistoryQuery.isReady();
}

bool StaticSiteServer::serveUri(const String& uri)
{
    if (!storageReady() || !isSafeUri(uri))
    {
        return false;
    }

    String path;
    if (!resolvePath(uri, path))
    {
        return false;
    }

    return streamFile(path);
}

bool StaticSiteServer::resolvePath(const String& uri, String& resolvedPath) const
{
    String clean = uri;
    const int query = clean.indexOf('?');
    if (query >= 0)
    {
        clean.remove(query);
    }

    if (clean.length() == 0U)
    {
        clean = "/";
    }

    String candidate = m_webRoot + clean;
    if (candidate.endsWith("/"))
    {
        candidate += "index.html";
    }

    if (candidate.length() >= MaxPathLength)
    {
        return false;
    }

    if (m_storage.exists(candidate))
    {
        resolvedPath = candidate;
        return true;
    }

    return tryVariantFallback(candidate, resolvedPath);
}

bool StaticSiteServer::tryVariantFallback(const String& requestedPath, String& resolvedPath) const
{
    String alternate = requestedPath;

    if (alternate.indexOf("/mobile/") >= 0)
    {
        alternate.replace("/mobile/", "/common/");
        if (m_storage.exists(alternate))
        {
            resolvedPath = alternate;
            return true;
        }

        alternate = requestedPath;
        alternate.replace("/mobile/", "/desktop/");
        if (m_storage.exists(alternate))
        {
            resolvedPath = alternate;
            return true;
        }
    }
    else if (alternate.indexOf("/desktop/") >= 0)
    {
        alternate.replace("/desktop/", "/common/");
        if (m_storage.exists(alternate))
        {
            resolvedPath = alternate;
            return true;
        }

        alternate = requestedPath;
        alternate.replace("/desktop/", "/mobile/");
        if (m_storage.exists(alternate))
        {
            resolvedPath = alternate;
            return true;
        }
    }

    return false;
}

bool StaticSiteServer::streamFile(const String& path)
{
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    m_server.sendHeader("Cache-Control", "no-cache");
    m_server.streamFile(file, contentTypeFor(path));
    file.close();
    return true;
}

bool StaticSiteServer::isSafeUri(const String& uri) const
{
    if (!uri.startsWith("/") || uri.indexOf("..") >= 0 || uri.indexOf('\\') >= 0)
    {
        return false;
    }

    // API routes must always be handled by their dedicated handlers and must
    // never be interpreted as file names on the card.
    return !uri.startsWith("/api/");
}

const char* StaticSiteServer::contentTypeFor(const String& path) const
{
    if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html; charset=utf-8";
    if (path.endsWith(".css")) return "text/css; charset=utf-8";
    if (path.endsWith(".js")) return "application/javascript; charset=utf-8";
    if (path.endsWith(".json")) return "application/json; charset=utf-8";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".gif")) return "image/gif";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".webp")) return "image/webp";
    if (path.endsWith(".txt")) return "text/plain; charset=utf-8";
    if (path.endsWith(".pdf")) return "application/pdf";
    if (path.endsWith(".woff")) return "font/woff";
    if (path.endsWith(".woff2")) return "font/woff2";
    return "application/octet-stream";
}

void StaticSiteServer::redirect(const char* location)
{
    m_server.sendHeader("Location", location != nullptr ? location : "/");
    m_server.send(302, "text/plain", "");
}
}
