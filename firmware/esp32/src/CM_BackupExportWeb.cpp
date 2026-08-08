#include "CM_BackupExportWeb.h"
#include "CM_BackupActivityGuard.h"
#include "CM_WarehouseMovementIntegrityAudit.h"
#include "CM_WarehousePersistenceIntegrityAudit.h"
#include "CM_MaterialPersistenceIntegrityAudit.h"
#include "CM_BackupBusinessDataIntegrityAudit.h"
#include "CM_WindingPersistenceIntegrityAudit.h"
#include "CM_WindingSessionPersistenceIntegrityAudit.h"
#include "CM_PersistentIdIntegrityAudit.h"
#include "CM_ConductorSettingsIntegrityAudit.h"

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

struct RecoveryMarkerDefinition
{
    const char* path;
    const char* reason;
};

struct SnapshotAuditMetrics
{
    bool materialRecordCountsMeasured = false;
    uint32_t materialCatalogRecordCount = 0UL;
    uint32_t materialUsageRecordCount = 0UL;
    uint32_t materialAdjustmentRecordCount = 0UL;
    bool businessRecordCountsMeasured = false;
    uint32_t workshopClientRecordCount = 0UL;
    uint32_t workshopMotorRecordCount = 0UL;
    uint32_t workshopRepairRecordCount = 0UL;
    uint32_t repairStatusRecordCount = 0UL;
    uint32_t repairPricingRecordCount = 0UL;
    bool windingJournalRecordCountMeasured = false;
    uint32_t windingJournalRecordCount = 0UL;
    bool windingSessionFileCountsMeasured = false;
    uint32_t windingSnapshotFileCount = 0UL;
    uint32_t windingStateFileCount = 0UL;
    uint32_t windingSpoolSelectionFileCount = 0UL;
    bool warehousePersistenceRecordCountsMeasured = false;
    uint32_t warehouseSpoolRecordCount = 0UL;
    uint32_t warehousePriceRecordCount = 0UL;
    bool warehouseMovementRecordCountMeasured = false;
    uint32_t warehouseMovementRecordCount = 0UL;
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
    {"winding-id-state-backup", "/data/winding-jobs/id-state.bak", "text/plain; charset=utf-8", "winding-id-state.bak"},
    {"conductor-settings", "/data/settings/conductor-calculator.ndjson", "application/x-ndjson", "conductor-calculator.ndjson"}
};

constexpr RecoveryMarkerDefinition RecoveryMarkers[] =
{
    {"/data/materials/usage.pending", "material_usage_pending"},
    {"/data/materials/adjustment.pending", "material_adjustment_pending"},
    {"/data/materials/materials.tmp", "material_swap_temp_present"},
    {"/data/materials/materials.bak", "material_swap_backup_present"},
    {"/data/warehouse/spools.tmp", "warehouse_spool_swap_temp_present"},
    {"/data/warehouse/spools.bak", "warehouse_spool_swap_backup_present"}
};

constexpr size_t ExportFileCount = sizeof(ExportFiles) / sizeof(ExportFiles[0]);
constexpr size_t RecoveryMarkerCount = sizeof(RecoveryMarkers) / sizeof(RecoveryMarkers[0]);
constexpr const char* SnapshotDirectory = "/data/winding-jobs/snapshots";
constexpr const char* SpoolSelectionDirectory = "/data/winding-jobs/spool-selection";
constexpr const char* StateDirectory = "/data/winding-jobs/state";
constexpr const char* WarehouseMovementsPath = "/data/warehouse/movements.ndjson";
constexpr uint8_t MaxSessionPage = 32U;

const ExportFileDefinition* findDefinition(const String& name)
{
    for (size_t i = 0U; i < ExportFileCount; ++i)
    {
        if (name == ExportFiles[i].name) return &ExportFiles[i];
    }
    return nullptr;
}

bool parseCanonicalUint32(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < source.length(); ++i)
    {
        const char ch = source[i];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}

String baseNameOf(const String& path)
{
    const int separator = path.lastIndexOf('/');
    return separator >= 0 ? path.substring(separator + 1) : path;
}

bool parseSessionJsonName(const String& fileName, uint32_t& sessionId)
{
    sessionId = 0UL;
    const String baseName = baseNameOf(fileName);
    if (!baseName.startsWith(F("session-")) || !baseName.endsWith(F(".json")))
        return false;
    const size_t prefixLength = 8U;
    const size_t suffixLength = 5U;
    if (baseName.length() <= prefixLength + suffixLength) return false;
    const String idText = baseName.substring(prefixLength,
                                             baseName.length() - suffixLength);
    return parseCanonicalUint32(idText, sessionId) && sessionId != 0UL;
}

bool isCanonicalTempName(const String& fileName)
{
    const String baseName = baseNameOf(fileName);
    if (!baseName.startsWith(F("session-")) || !baseName.endsWith(F(".tmp")))
        return false;
    const String idText = baseName.substring(8U, baseName.length() - 4U);
    uint32_t sessionId = 0UL;
    return parseCanonicalUint32(idText, sessionId) && sessionId != 0UL;
}

void insertSortedUnique(uint32_t value,
                        uint32_t* values,
                        uint8_t& count,
                        uint8_t capacity,
                        bool& truncated)
{
    if (value == 0UL || values == nullptr || capacity == 0U) return;
    uint8_t position = 0U;
    while (position < count && values[position] < value) ++position;
    if (position < count && values[position] == value) return;

    if (count < capacity)
    {
        for (uint8_t i = count; i > position; --i)
            values[i] = values[i - 1U];
        values[position] = value;
        ++count;
        return;
    }

    truncated = true;
    if (position >= capacity) return;
    for (uint8_t i = static_cast<uint8_t>(capacity - 1U); i > position; --i)
        values[i] = values[i - 1U];
    values[position] = value;
}

enum class SessionScanResult : uint8_t
{
    Ok = 0U,
    StorageUnavailable,
    TemporaryFilePresent,
    InvalidEntry
};

SessionScanResult scanSessionDirectory(fs::FS& storage,
                                       const char* directoryPath,
                                       uint32_t afterSessionId,
                                       uint32_t* ids,
                                       uint8_t& count,
                                       uint8_t capacity,
                                       bool& truncated)
{
    if (!storage.exists(directoryPath)) return SessionScanResult::Ok;
    File directory = storage.open(directoryPath, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return SessionScanResult::StorageUnavailable;
    }

    File entry = directory.openNextFile();
    while (entry)
    {
        const String name = entry.name();
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return SessionScanResult::InvalidEntry;
        }

        if (isCanonicalTempName(name))
        {
            entry.close();
            directory.close();
            return SessionScanResult::TemporaryFilePresent;
        }

        uint32_t sessionId = 0UL;
        if (!parseSessionJsonName(name, sessionId))
        {
            entry.close();
            directory.close();
            return SessionScanResult::InvalidEntry;
        }
        if (sessionId > afterSessionId)
            insertSortedUnique(sessionId, ids, count, capacity, truncated);

        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return SessionScanResult::Ok;
}

const char* snapshotStabilityReason(fs::FS& storage,
                                    SnapshotAuditMetrics& metrics)
{
    metrics.materialRecordCountsMeasured = false;
    metrics.materialCatalogRecordCount = 0UL;
    metrics.materialUsageRecordCount = 0UL;
    metrics.materialAdjustmentRecordCount = 0UL;
    metrics.businessRecordCountsMeasured = false;
    metrics.workshopClientRecordCount = 0UL;
    metrics.workshopMotorRecordCount = 0UL;
    metrics.workshopRepairRecordCount = 0UL;
    metrics.repairStatusRecordCount = 0UL;
    metrics.repairPricingRecordCount = 0UL;
    metrics.windingJournalRecordCountMeasured = false;
    metrics.windingJournalRecordCount = 0UL;
    metrics.windingSessionFileCountsMeasured = false;
    metrics.windingSnapshotFileCount = 0UL;
    metrics.windingStateFileCount = 0UL;
    metrics.windingSpoolSelectionFileCount = 0UL;
    metrics.warehousePersistenceRecordCountsMeasured = false;
    metrics.warehouseSpoolRecordCount = 0UL;
    metrics.warehousePriceRecordCount = 0UL;
    metrics.warehouseMovementRecordCountMeasured = false;
    metrics.warehouseMovementRecordCount = 0UL;

    for (size_t i = 0U; i < RecoveryMarkerCount; ++i)
    {
        if (storage.exists(RecoveryMarkers[i].path))
            return RecoveryMarkers[i].reason;
    }

    if (!PersistentIdIntegrityAudit::check(storage))
        return "persistent_id_unstable_or_invalid";

    if (!ConductorSettingsIntegrityAudit::check(storage))
        return "conductor_settings_unstable_or_invalid";

    MaterialPersistenceAuditMetrics materialMetrics;
    if (!MaterialPersistenceIntegrityAudit::check(storage, materialMetrics))
        return "material_persistence_unstable_or_invalid";
    metrics.materialCatalogRecordCount = materialMetrics.materialRecordCount;
    metrics.materialUsageRecordCount = materialMetrics.usageRecordCount;
    metrics.materialAdjustmentRecordCount = materialMetrics.adjustmentRecordCount;
    metrics.materialRecordCountsMeasured = true;

    BackupBusinessDataAuditMetrics businessMetrics;
    if (!BackupBusinessDataIntegrityAudit::check(storage, businessMetrics))
        return "business_data_unstable_or_invalid";
    metrics.workshopClientRecordCount = businessMetrics.clientRecordCount;
    metrics.workshopMotorRecordCount = businessMetrics.motorRecordCount;
    metrics.workshopRepairRecordCount = businessMetrics.repairRecordCount;
    metrics.repairStatusRecordCount = businessMetrics.repairStatusRecordCount;
    metrics.repairPricingRecordCount = businessMetrics.pricingRecordCount;
    metrics.businessRecordCountsMeasured = true;

    uint32_t windingJournalRecordCount = 0UL;
    if (!WindingPersistenceIntegrityAudit::check(storage,
                                                 windingJournalRecordCount))
    {
        return "winding_persistence_unstable_or_invalid";
    }
    metrics.windingJournalRecordCount = windingJournalRecordCount;
    metrics.windingJournalRecordCountMeasured = true;

    WarehousePersistenceAuditMetrics warehouseMetrics;
    if (!WarehousePersistenceIntegrityAudit::check(storage, warehouseMetrics))
        return "warehouse_persistence_unstable_or_invalid";
    metrics.warehouseSpoolRecordCount = warehouseMetrics.spoolRecordCount;
    metrics.warehousePriceRecordCount = warehouseMetrics.priceRecordCount;
    metrics.warehousePersistenceRecordCountsMeasured = true;

    if (storage.exists(WarehouseMovementsPath))
    {
        uint32_t warehouseMovementRecordCount = 0UL;
        if (!WarehouseMovementIntegrityAudit::check(storage,
                                                    warehouseMovementRecordCount))
        {
            return "warehouse_movements_unstable_or_invalid";
        }
        metrics.warehouseMovementRecordCount = warehouseMovementRecordCount;
        metrics.warehouseMovementRecordCountMeasured = true;
    }

    const char* directories[] =
    {
        SnapshotDirectory,
        SpoolSelectionDirectory,
        StateDirectory
    };
    uint32_t ignoredIds[1] = {};
    for (uint8_t i = 0U; i < sizeof(directories) / sizeof(directories[0]); ++i)
    {
        uint8_t count = 0U;
        bool truncated = false;
        const SessionScanResult result =
            scanSessionDirectory(storage, directories[i], 0UL,
                                 ignoredIds, count, 1U, truncated);
        if (result == SessionScanResult::StorageUnavailable)
            return "session_directory_unavailable";
        if (result == SessionScanResult::TemporaryFilePresent)
            return "session_temp_present";
        if (result == SessionScanResult::InvalidEntry)
            return "session_directory_invalid";
    }

    WindingSessionPersistenceAuditMetrics windingSessionMetrics;
    if (!WindingSessionPersistenceIntegrityAudit::check(storage,
                                                        windingSessionMetrics))
    {
        return "winding_session_persistence_unstable_or_invalid";
    }
    metrics.windingSnapshotFileCount = windingSessionMetrics.snapshotFileCount;
    metrics.windingStateFileCount = windingSessionMetrics.stateFileCount;
    metrics.windingSpoolSelectionFileCount =
        windingSessionMetrics.spoolSelectionFileCount;
    metrics.windingSessionFileCountsMeasured = true;

    return nullptr;
}

String sessionPath(const char* kind, uint32_t sessionId)
{
    String path;
    if (strcmp(kind, "snapshot") == 0)
        path = F("/data/winding-jobs/snapshots/session-");
    else if (strcmp(kind, "spool-selection") == 0)
        path = F("/data/winding-jobs/spool-selection/session-");
    else
        path = F("/data/winding-jobs/state/session-");
    path += sessionId;
    path += F(".json");
    return path;
}

bool loadFileSize(fs::FS& storage,
                  const String& path,
                  bool& exists,
                  uint32_t& sizeBytes)
{
    exists = storage.exists(path);
    sizeBytes = 0UL;
    if (!exists) return true;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory() || file.size() > 0xFFFFFFFFUL)
    {
        if (file) file.close();
        return false;
    }
    sizeBytes = static_cast<uint32_t>(file.size());
    file.close();
    return true;
}

bool requireSafeExport(WebServer& server, fs::FS& storage)
{
    const BackupActivityCheck check = BackupActivityGuard::check(storage);
    if (check == BackupActivityCheck::Safe) return true;
    if (check == BackupActivityCheck::Busy)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"error\":\"backup_blocked_while_winding_active\"}");
        return false;
    }
    server.send(503, "application/json; charset=utf-8",
                "{\"error\":\"backup_activity_state_unavailable\"}");
    return false;
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
    m_server.on("/api/backup/sessions", HTTP_GET,
                [this]() { handleSessions(); });
    m_server.on("/api/backup/session-file", HTTP_GET,
                [this]() { handleSessionFile(); });
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

    const BackupActivityCheck activity = BackupActivityGuard::check(m_storage);
    const bool stabilityChecked = activity == BackupActivityCheck::Safe;
    const char* stabilityReason = nullptr;
    uint32_t stabilityDurationMs = 0UL;
    SnapshotAuditMetrics auditMetrics;
    if (stabilityChecked)
    {
        const uint32_t startedAtMs = millis();
        stabilityReason = snapshotStabilityReason(m_storage, auditMetrics);
        stabilityDurationMs = millis() - startedAtMs;
    }

    String response = F("{\"read_only\":true,\"arbitrary_paths_allowed\":false,\"session_exports_supported\":true,\"spool_selection_exports_supported\":true,\"export_allowed\":");
    response += activity == BackupActivityCheck::Safe ? F("true") : F("false");
    response += F(",\"activity_state_verified\":");
    response += activity != BackupActivityCheck::Unavailable ? F("true") : F("false");
    response += F(",\"blocked_reason\":");
    if (activity == BackupActivityCheck::Busy)
        response += F("\"winding_active\"");
    else if (activity == BackupActivityCheck::Unavailable)
        response += F("\"activity_state_unavailable\"");
    else
        response += F("null");
    response += F(",\"snapshot_stability_checked\":");
    response += stabilityChecked ? F("true") : F("false");
    response += F(",\"snapshot_stable\":");
    if (!stabilityChecked)
        response += F("null");
    else
        response += stabilityReason == nullptr ? F("true") : F("false");
    response += F(",\"snapshot_stability_reason\":");
    if (!stabilityChecked || stabilityReason == nullptr)
        response += F("null");
    else
    {
        response += '"';
        response += stabilityReason;
        response += '"';
    }
    response += F(",\"snapshot_stability_duration_ms\":");
    if (!stabilityChecked)
        response += F("null");
    else
        response += stabilityDurationMs;
    response += F(",\"material_catalog_record_count\":");
    if (!stabilityChecked || !auditMetrics.materialRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.materialCatalogRecordCount;
    response += F(",\"material_usage_record_count\":");
    if (!stabilityChecked || !auditMetrics.materialRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.materialUsageRecordCount;
    response += F(",\"material_adjustment_record_count\":");
    if (!stabilityChecked || !auditMetrics.materialRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.materialAdjustmentRecordCount;
    response += F(",\"workshop_client_record_count\":");
    if (!stabilityChecked || !auditMetrics.businessRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.workshopClientRecordCount;
    response += F(",\"workshop_motor_record_count\":");
    if (!stabilityChecked || !auditMetrics.businessRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.workshopMotorRecordCount;
    response += F(",\"workshop_repair_record_count\":");
    if (!stabilityChecked || !auditMetrics.businessRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.workshopRepairRecordCount;
    response += F(",\"repair_status_record_count\":");
    if (!stabilityChecked || !auditMetrics.businessRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.repairStatusRecordCount;
    response += F(",\"repair_pricing_record_count\":");
    if (!stabilityChecked || !auditMetrics.businessRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.repairPricingRecordCount;
    response += F(",\"winding_journal_record_count\":");
    if (!stabilityChecked || !auditMetrics.windingJournalRecordCountMeasured)
        response += F("null");
    else
        response += auditMetrics.windingJournalRecordCount;
    response += F(",\"winding_snapshot_file_count\":");
    if (!stabilityChecked || !auditMetrics.windingSessionFileCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.windingSnapshotFileCount;
    response += F(",\"winding_state_file_count\":");
    if (!stabilityChecked || !auditMetrics.windingSessionFileCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.windingStateFileCount;
    response += F(",\"winding_spool_selection_file_count\":");
    if (!stabilityChecked || !auditMetrics.windingSessionFileCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.windingSpoolSelectionFileCount;
    response += F(",\"warehouse_spool_record_count\":");
    if (!stabilityChecked || !auditMetrics.warehousePersistenceRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.warehouseSpoolRecordCount;
    response += F(",\"warehouse_price_record_count\":");
    if (!stabilityChecked || !auditMetrics.warehousePersistenceRecordCountsMeasured)
        response += F("null");
    else
        response += auditMetrics.warehousePriceRecordCount;
    response += F(",\"warehouse_movement_record_count\":");
    if (!stabilityChecked || !auditMetrics.warehouseMovementRecordCountMeasured)
        response += F("null");
    else
        response += auditMetrics.warehouseMovementRecordCount;
    response += F(",\"items\":[");
    response.reserve(5000U);
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
    response += F(",\"sessions_endpoint\":\"/api/backup/sessions\",\"session_file_endpoint\":\"/api/backup/session-file\"}");
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
    if (!requireSafeExport(m_server, m_storage)) return;
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

void BackupExportWeb::handleSessions()
{
    if (!ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"backup_storage_unavailable\"}");
        return;
    }
    if (!requireSafeExport(m_server, m_storage)) return;

    uint32_t afterSessionId = 0UL;
    if (m_server.hasArg("after_session_id") &&
        m_server.arg("after_session_id").length() > 0U &&
        !parseCanonicalUint32(m_server.arg("after_session_id"), afterSessionId))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_after_session_id\"}");
        return;
    }

    uint32_t limitValue = MaxSessionPage;
    if (m_server.hasArg("limit") && m_server.arg("limit").length() > 0U)
    {
        if (!parseCanonicalUint32(m_server.arg("limit"), limitValue) ||
            limitValue == 0UL || limitValue > MaxSessionPage)
        {
            m_server.send(400, "application/json; charset=utf-8",
                          "{\"error\":\"invalid_backup_session_limit\"}");
            return;
        }
    }

    uint32_t ids[MaxSessionPage] = {};
    uint8_t count = 0U;
    bool truncated = false;
    const uint8_t capacity = static_cast<uint8_t>(limitValue);
    const SessionScanResult snapshotScan =
        scanSessionDirectory(m_storage, SnapshotDirectory, afterSessionId,
                             ids, count, capacity, truncated);
    const SessionScanResult spoolSelectionScan =
        snapshotScan == SessionScanResult::Ok
            ? scanSessionDirectory(m_storage, SpoolSelectionDirectory, afterSessionId,
                                   ids, count, capacity, truncated)
            : snapshotScan;
    const SessionScanResult stateScan =
        snapshotScan == SessionScanResult::Ok &&
        spoolSelectionScan == SessionScanResult::Ok
            ? scanSessionDirectory(m_storage, StateDirectory, afterSessionId,
                                   ids, count, capacity, truncated)
            : spoolSelectionScan;
    SessionScanResult scanResult = snapshotScan;
    if (scanResult == SessionScanResult::Ok) scanResult = spoolSelectionScan;
    if (scanResult == SessionScanResult::Ok) scanResult = stateScan;

    if (scanResult == SessionScanResult::StorageUnavailable)
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"backup_session_storage_unavailable\"}");
        return;
    }
    if (scanResult == SessionScanResult::TemporaryFilePresent)
    {
        m_server.send(409, "application/json; charset=utf-8",
                      "{\"error\":\"backup_session_directory_unstable\"}");
        return;
    }
    if (scanResult != SessionScanResult::Ok)
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"backup_session_directory_invalid\"}");
        return;
    }

    String response = F("{\"items\":[");
    response.reserve(5120U);
    for (uint8_t i = 0U; i < count; ++i)
    {
        const uint32_t sessionId = ids[i];
        const String snapshot = sessionPath("snapshot", sessionId);
        const String spoolSelection = sessionPath("spool-selection", sessionId);
        const String state = sessionPath("state", sessionId);
        bool snapshotExists = false;
        bool spoolSelectionExists = false;
        bool stateExists = false;
        uint32_t snapshotSize = 0UL;
        uint32_t spoolSelectionSize = 0UL;
        uint32_t stateSize = 0UL;

        if (!loadFileSize(m_storage, snapshot, snapshotExists, snapshotSize) ||
            !loadFileSize(m_storage, spoolSelection, spoolSelectionExists, spoolSelectionSize) ||
            !loadFileSize(m_storage, state, stateExists, stateSize))
        {
            m_server.send(500, "application/json; charset=utf-8",
                          "{\"error\":\"backup_session_file_read_failed\"}");
            return;
        }

        if (i > 0U) response += ',';
        response += F("{\"session_id\":");
        response += sessionId;
        response += F(",\"snapshot_exists\":");
        response += snapshotExists ? F("true") : F("false");
        response += F(",\"snapshot_size_bytes\":");
        response += snapshotSize;
        response += F(",\"spool_selection_exists\":");
        response += spoolSelectionExists ? F("true") : F("false");
        response += F(",\"spool_selection_size_bytes\":");
        response += spoolSelectionSize;
        response += F(",\"state_exists\":");
        response += stateExists ? F("true") : F("false");
        response += F(",\"state_size_bytes\":");
        response += stateSize;
        response += '}';
    }
    response += F("],\"count\":");
    response += count;
    response += F(",\"after_session_id\":");
    response += afterSessionId;
    response += F(",\"next_after_session_id\":");
    response += count > 0U ? ids[count - 1U] : afterSessionId;
    response += F(",\"has_more\":");
    response += truncated ? F("true") : F("false");
    response += '}';
    m_server.sendHeader("Cache-Control", "no-store");
    m_server.send(200, "application/json; charset=utf-8", response);
}

void BackupExportWeb::handleSessionFile()
{
    if (!ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"backup_storage_unavailable\"}");
        return;
    }
    if (!requireSafeExport(m_server, m_storage)) return;
    if (!m_server.hasArg("kind") || !m_server.hasArg("session_id"))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"backup_kind_and_session_id_required\"}");
        return;
    }

    const String kind = m_server.arg("kind");
    if (kind != "snapshot" && kind != "spool-selection" && kind != "state")
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_backup_session_kind\"}");
        return;
    }
    uint32_t sessionId = 0UL;
    if (!parseCanonicalUint32(m_server.arg("session_id"), sessionId) ||
        sessionId == 0UL)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_backup_session_id\"}");
        return;
    }

    const String path = sessionPath(kind.c_str(), sessionId);
    if (!m_storage.exists(path))
    {
        m_server.send(404, "application/json; charset=utf-8",
                      "{\"error\":\"backup_session_file_not_found\"}");
        return;
    }
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"backup_session_file_read_failed\"}");
        return;
    }

    String disposition = F("attachment; filename=\"");
    disposition += kind;
    disposition += F("-session-");
    disposition += sessionId;
    disposition += F(".json\"");
    m_server.sendHeader("Content-Disposition", disposition);
    m_server.sendHeader("Cache-Control", "no-store");
    m_server.streamFile(file, "application/json; charset=utf-8");
    file.close();
}
}
