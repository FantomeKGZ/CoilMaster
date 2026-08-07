#include "CM_BackupExportWeb.h"

namespace CM
{
namespace
{
struct ExportFileDefinition
{
    const char* name;
    const char* path;
    const char* contentType;
    const char* downloadName;
};

constexpr ExportFileDefinition ExportFiles[] =
{
    {"workshop-clients", "/data/workshop/clients.ndjson", "application/x-ndjson", "clients.ndjson"},
    {"workshop-motors", "/data/workshop/motors.ndjson", "application/x-ndjson", "motors.ndjson"},
    {"workshop-repairs", "/data/workshop/repairs.ndjson", "application/x-ndjson", "repairs.ndjson"},
    {"repair-status", "/data/workshop/repair-status.ndjson", "application/x-ndjson", "repair-status.ndjson"},
    {"winding-events", "/data/winding-runs/events.ndjson", "application/x-ndjson", "winding-events.ndjson"},
    {"warehouse-spools", "/data/warehouse/spools.ndjson", "application/x-ndjson", "warehouse-spools.ndjson"},
    {"warehouse-movements", "/data/warehouse/movements.ndjson", "application/x-ndjson", "warehouse-movements.ndjson"},
    {"warehouse-price", "/data/warehouse/price.ndjson", "application/x-ndjson", "warehouse-price.ndjson"},
    {"materials", "/data/materials/materials.ndjson", "application/x-ndjson", "materials.ndjson"},
    {"material-usage", "/data/materials/usage.ndjson", "application/x-ndjson", "material-usage.ndjson"},
    {"material-adjustments", "/data/materials/adjustments.ndjson", "application/x-ndjson", "material-adjustments.ndjson"},
    {"repair-pricing", "/data/repairs/pricing.ndjson", "application/x-ndjson", "repair-pricing.ndjson"},
    {"winding-id-state", "/data/winding-jobs/id-state.txt", "text/plain; charset=utf-8", "winding-id-state.txt"},
    {"conductor-settings", "/data/settings/conductor.json", "application/json; charset=utf-8", "conductor-settings.json"}
};

constexpr size_t ExportFileCount = sizeof(ExportFiles) / sizeof(ExportFiles[0]);

const ExportFileDefinition* findDefinition(const String& name)
{
    for (size_t i = 0U; i < ExportFileCount; ++i)
    {
        if (name == ExportFiles[i].name) return &ExportFiles[i];
    }
    return nullptr;
}
}

BackupExportWeb::BackupExportWeb(WebServer& server, fs::FS& storage)
    : m_server(server), m_storage(storage) {}

void BackupExportWeb::begin()
{
    m_server.on("/api/backup/manifest", HTTP_GET,
                [this]() { handleManifest(); });
    m_server.on("/api/backup/file", HTTP_GET,
                [this]() { handleFile(); });
}

bool BackupExportWeb::ready() const
{
    File root = m_storage.open("/data", FILE_READ);
    if (!root) return false;
    const bool available = root.isDirectory();
    root.close();
    return available;
}

void BackupExportWeb::handleManifest()
{
    if (!ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"backup_storage_unavailable\"}");
        return;
    }

    String response = F("{\"read_only\":true,\"arbitrary_paths_allowed\":false,\"items\":[");
    response.reserve(3072U);
    bool first = true;

    for (size_t i = 0U; i < ExportFileCount; ++i)
    {
        const ExportFileDefinition& definition = ExportFiles[i];
        bool exists = false;
        uint32_t sizeBytes = 0UL;

        if (m_storage.exists(definition.path))
        {
            File file = m_storage.open(definition.path, FILE_READ);
            if (!file || file.isDirectory())
            {
                if (file) file.close();
                m_server.send(500, "application/json; charset=utf-8",
                              "{\"error\":\"backup_manifest_read_failed\"}");
                return;
            }
            const size_t fileSize = file.size();
            file.close();
            if (fileSize > 0xFFFFFFFFUL)
            {
                m_server.send(500, "application/json; charset=utf-8",
                              "{\"error\":\"backup_file_too_large\"}");
                return;
            }
            exists = true;
            sizeBytes = static_cast<uint32_t>(fileSize);
        }

        if (!first) response += ',';
        first = false;
        response += F("{\"name\":\"");
        response += definition.name;
        response += F("\",\"download_name\":\"");
        response += definition.downloadName;
        response += F("\",\"exists\":");
        response += exists ? F("true") : F("false");
        response += F(",\"size_bytes\":");
        response += sizeBytes;
        response += '}';
    }

    response += F("],\"count\":");
    response += static_cast<uint32_t>(ExportFileCount);
    response += F("}");
    m_server.sendHeader("Cache-Control", "no-store");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void BackupExportWeb::handleFile()
{
    if (!ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"backup_storage_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("name") || m_server.arg("name").length() == 0U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"backup_name_required\"}");
        return;
    }

    const ExportFileDefinition* definition = findDefinition(m_server.arg("name"));
    if (definition == nullptr)
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"backup_file_not_allowed\"}");
        return;
    }
    if (!m_storage.exists(definition->path))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"backup_file_not_found\"}");
        return;
    }

    File file = m_storage.open(definition->path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"backup_file_read_failed\"}");
        return;
    }

    String disposition = F("attachment; filename=\"");
    disposition += definition->downloadName;
    disposition += '"';
    m_server.sendHeader("Content-Disposition", disposition);
    m_server.sendHeader("Cache-Control", "no-store");
    m_server.streamFile(file, definition->contentType);
    file.close();
}
}
