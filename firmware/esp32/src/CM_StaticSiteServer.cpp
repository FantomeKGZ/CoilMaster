#include "CM_StaticSiteServer.h"

#include <WiFi.h>

#include "CM_JobDisplayRecovery.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_JobStateStore.h"

namespace
{
constexpr size_t HtmlStreamChunkSize = 512U;

const char UiVersionSwitchScript[] PROGMEM = R"HTML(
<script>
(()=>{
  if(document.getElementById('cm-version-switch'))return;
  const p=location.pathname;
  const m=p.match(/^\/(mobile|desktop)(\/.*)?$/);
  if(!m)return;
  const current=m[1],target=current==='mobile'?'desktop':'mobile',rest=m[2]||'/';
  const box=document.createElement('div');
  box.id='cm-version-switch';
  box.style.cssText='box-sizing:border-box;max-width:760px;margin:18px auto '+(current==='mobile'?'94px':'26px')+';padding:0 12px;position:relative;z-index:10';
  const a=document.createElement('a');
  a.href='/'+target+rest+location.search+location.hash;
  a.style.cssText='display:block;box-sizing:border-box;padding:14px 16px;border:1px solid #b9cddd;border-radius:12px;background:#e8f1f8;color:#105b91;text-align:center;text-decoration:none;font:700 16px Arial,sans-serif;box-shadow:0 2px 8px #17212b12';
  a.textContent=current==='mobile'?'🖥️ Открыть эту страницу в версии для ПК':'📱 Вернуться на эту страницу в мобильной версии';
  a.addEventListener('click',()=>localStorage.setItem('cm-ui-version',target));
  box.appendChild(a);
  document.body.appendChild(box);
  if(rest==='/backup.html'){
    const helper=document.createElement('script');
    helper.src='/shared/backup-remote-upload.js';
    document.body.appendChild(helper);
  }
})();
</script>
)HTML";

bool parseCanonicalUint32Value(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    value = parsed;
    return true;
}

const char* wifiModeName(wifi_mode_t mode)
{
    switch (mode)
    {
        case WIFI_MODE_STA: return "STA";
        case WIFI_MODE_AP: return "AP";
        case WIFI_MODE_APSTA: return "AP_STA";
        case WIFI_MODE_NULL:
        default: return "OFF";
    }
}

bool isUiVariantHtml(const String& uri, const String& path)
{
    if (!path.endsWith(".html") && !path.endsWith(".htm")) return false;
    return uri == "/mobile" || uri.startsWith("/mobile/") ||
           uri == "/desktop" || uri.startsWith("/desktop/");
}

bool streamHtmlWithUiSwitch(WebServer& server, File& file)
{
    server.sendHeader("Cache-Control", "no-cache");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html; charset=utf-8", "");

    char buffer[HtmlStreamChunkSize];
    while (file.available())
    {
        const size_t read = file.read(reinterpret_cast<uint8_t*>(buffer),
                                      sizeof(buffer));
        if (read == 0U) return false;
        server.sendContent(buffer, read);
    }

    server.sendContent_P(UiVersionSwitchScript,
                         sizeof(UiVersionSwitchScript) - 1U);
    return true;
}
}

namespace CM
{
StaticSiteServer::StaticSiteServer(WebServer& server,
                                   fs::FS& storage,
                                   NetworkManager& networkManager)
    : m_server(server),
      m_storage(storage),
      m_windingHistoryQuery(storage),
      m_windingHistoryWeb(server, m_windingHistoryQuery),
      m_networkManager(networkManager),
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

    // Runtime-only status. Profile secrets remain in the separate bounded API.
    m_server.on("/api/system/network", HTTP_GET, [this]()
    {
        const wifi_mode_t mode = WiFi.getMode();
        String response = F("{\"mode\":\"");
        response += wifiModeName(mode);
        response += F("\",\"network_state\":\"");
        response += m_networkManager.stateName();
        response += F("\",\"network_last_result\":\"");
        response += m_networkManager.lastResult();
        response += F("\",\"active_profile_id\":");
        if (m_networkManager.activeProfileId() != 0U)
            response += m_networkManager.activeProfileId();
        else response += F("null");
        response += F(",\"sta_connecting\":");
        response += m_networkManager.connecting() ? F("true") : F("false");
        response += F(",\"ap_active\":");
        response += (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) ? F("true") : F("false");
        response += F(",\"ap_ssid\":\"");
        response += WiFi.softAPSSID();
        response += F("\",\"ap_ip\":\"");
        response += WiFi.softAPIP().toString();
        response += F("\",\"sta_connected\":");
        response += WiFi.status() == WL_CONNECTED ? F("true") : F("false");
        response += F(",\"sta_ssid\":\"");
        if (WiFi.status() == WL_CONNECTED) response += WiFi.SSID();
        response += F("\",\"sta_ip\":\"");
        if (WiFi.status() == WL_CONNECTED) response += WiFi.localIP().toString();
        response += F("\",\"sta_rssi\":");
        response += WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
        response += F(",\"saved_profiles_supported\":true,\"static_ip_supported\":false,");
        response += F("\"ftp_supported\":false,\"ftp_enabled\":false}");
        m_server.send(200, "application/json; charset=utf-8", response);
    });

    // Operator-only recovery closure. The persisted state, immutable snapshot
    // and (for linked jobs) immutable exact spool selection are revalidated
    // immediately before closure. No automatic resume or physical START occurs.
    // Restart is intentional so no stale in-memory active job survives review.
    m_server.on("/api/recovery/acknowledge-and-restart", HTTP_POST, [this]()
    {
        if (!m_server.hasArg("session_id") || !m_server.hasArg("confirmed"))
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"session_id_and_confirmation_required\"}");
            return;
        }
        if (m_server.arg("confirmed") != "true")
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"explicit_confirmation_required\"}");
            return;
        }

        uint32_t sessionId = 0UL;
        if (!parseCanonicalUint32Value(m_server.arg("session_id"), sessionId) ||
            sessionId == 0UL)
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"invalid_session_id\"}");
            return;
        }

        JobStateStore states(m_storage);
        if (!states.begin())
        {
            m_server.send(503, "application/json",
                          "{\"error\":\"job_state_store_unavailable\"}");
            return;
        }

        JobRuntimeState latest;
        bool found = false;
        if (!states.loadLatest(latest, found))
        {
            m_server.send(500, "application/json",
                          "{\"error\":\"job_state_integrity_failed\"}");
            return;
        }
        if (!found)
        {
            m_server.send(404, "application/json",
                          "{\"error\":\"job_state_not_found\"}");
            return;
        }
        if (latest.sessionId != sessionId)
        {
            m_server.send(409, "application/json",
                          "{\"error\":\"session_mismatch\"}");
            return;
        }

        JobSnapshotStore snapshots(m_storage);
        JobSnapshot snapshot;
        if (!snapshots.begin() ||
            !snapshots.load(latest.sessionId, snapshot) ||
            snapshot.jobId != latest.jobId)
        {
            m_server.send(500, "application/json",
                          "{\"error\":\"job_snapshot_identity_failed\"}");
            return;
        }

        if (snapshot.linkage.linked)
        {
            JobSpoolSelection selection;
            bool selectionFound = false;
            if (!JobSpoolSelectionStore::loadReadOnly(m_storage,
                                                       latest.sessionId,
                                                       selection,
                                                       selectionFound) ||
                !selectionFound || !selection.isValid() ||
                selection.jobId != latest.jobId ||
                selection.sessionId != latest.sessionId ||
                selection.repairId != snapshot.linkage.repairId ||
                selection.motorId != snapshot.linkage.motorId)
            {
                m_server.send(500, "application/json",
                              "{\"error\":\"job_spool_selection_identity_failed\"}");
                return;
            }
        }

        if (!states.closeAfterManualReview(sessionId, millis()))
        {
            m_server.send(409, "application/json",
                          "{\"error\":\"manual_review_not_required_or_state_changed\"}");
            return;
        }

        String response = F("{\"acknowledged\":true,\"session_id\":");
        response += sessionId;
        response += F(",\"state\":\"CLOSED_AFTER_REVIEW\",\"restarting\":true,");
        response += F("\"automatic_queue_allowed\":false,\"automatic_resume_allowed\":false}");
        m_server.send(200, "application/json; charset=utf-8", response);
        delay(350);
        ESP.restart();
    });

    // Removes only an already inactive unlinked/service job from the active
    // machine view. Immutable snapshots/history remain on storage. Accepted,
    // delivering and running jobs cannot be hidden through this route.
    m_server.on("/api/jobs/dismiss", HTTP_POST, [this]()
    {
        if (!m_server.hasArg("job_id") || !m_server.hasArg("session_id") ||
            !m_server.hasArg("confirmed"))
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"job_session_and_confirmation_required\"}");
            return;
        }
        if (m_server.arg("confirmed") != "true")
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"explicit_confirmation_required\"}");
            return;
        }

        uint32_t jobId = 0UL;
        uint32_t sessionId = 0UL;
        if (!parseCanonicalUint32Value(m_server.arg("job_id"), jobId) ||
            !parseCanonicalUint32Value(m_server.arg("session_id"), sessionId) ||
            jobId == 0UL || sessionId == 0UL)
        {
            m_server.send(400, "application/json",
                          "{\"error\":\"invalid_job_or_session_id\"}");
            return;
        }

        JobStateStore states(m_storage);
        if (!states.begin())
        {
            m_server.send(503, "application/json",
                          "{\"error\":\"job_state_store_unavailable\"}");
            return;
        }

        JobRuntimeState latest;
        bool found = false;
        if (!states.loadLatest(latest, found))
        {
            m_server.send(500, "application/json",
                          "{\"error\":\"job_state_integrity_failed\"}");
            return;
        }
        if (!found)
        {
            m_server.send(404, "application/json",
                          "{\"error\":\"job_state_not_found\"}");
            return;
        }
        if (latest.jobId != jobId || latest.sessionId != sessionId)
        {
            m_server.send(409, "application/json",
                          "{\"error\":\"job_or_session_mismatch\"}");
            return;
        }

        JobSnapshotStore snapshots(m_storage);
        if (!snapshots.begin())
        {
            m_server.send(503, "application/json",
                          "{\"error\":\"job_snapshot_store_unavailable\"}");
            return;
        }

        RecoveredJobDisplay display;
        if (!JobDisplayRecovery::load(snapshots, jobId, sessionId, display))
        {
            m_server.send(500, "application/json",
                          "{\"error\":\"job_snapshot_identity_failed\"}");
            return;
        }
        if (display.linkage.linked)
        {
            m_server.send(409, "application/json",
                          "{\"error\":\"linked_job_cannot_be_dismissed_here\"}");
            return;
        }

        if (!states.dismissInactive(sessionId, millis()))
        {
            m_server.send(409, "application/json",
                          "{\"error\":\"job_not_safely_inactive\"}");
            return;
        }

        String response = F("{\"dismissed\":true,\"job_id\":");
        response += jobId;
        response += F(",\"session_id\":");
        response += sessionId;
        response += F(",\"history_preserved\":true,\"restarting\":true}");
        m_server.send(200, "application/json; charset=utf-8", response);
        delay(350);
        ESP.restart();
    });

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

    if (isUiVariantHtml(m_server.uri(), path))
    {
        const bool streamed = streamHtmlWithUiSwitch(m_server, file);
        file.close();
        return streamed;
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
