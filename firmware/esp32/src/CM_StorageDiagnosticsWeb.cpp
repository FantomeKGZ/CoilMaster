#include "CM_StorageDiagnosticsWeb.h"

#include <Arduino.h>
#include <SD.h>

namespace
{
void appendUint64(String& target, uint64_t value)
{
    char buffer[24];
    snprintf(buffer,
             sizeof(buffer),
             "%llu",
             static_cast<unsigned long long>(value));
    target += buffer;
}

uint64_t readOnlyFileSize(fs::FS& storage, const char* path)
{
    if (path == nullptr || !storage.exists(path)) return 0ULL;
    File file = storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return 0ULL;
    }
    const uint64_t size = static_cast<uint64_t>(file.size());
    file.close();
    return size;
}
}

namespace CM
{
StorageDiagnosticsWeb::StorageDiagnosticsWeb(WebServer& server,
                                             fs::FS& storage)
    : m_server(server),
      m_storage(storage)
{
    m_server.on("/api/system/storage", HTTP_GET, [this]() { handleGet(); });
}

void StorageDiagnosticsWeb::handleGet()
{
    fs::SDFS& sd = static_cast<fs::SDFS&>(m_storage);
    const bool cardPresent = sd.cardType() != CARD_NONE;
    const uint64_t cardSize = cardPresent ? sd.cardSize() : 0ULL;
    const uint64_t totalBytes = cardPresent ? sd.totalBytes() : 0ULL;
    const uint64_t usedBytes = cardPresent ? sd.usedBytes() : 0ULL;
    const uint64_t freeBytes = totalBytes >= usedBytes
        ? totalBytes - usedBytes
        : 0ULL;
    const bool ready = cardPresent && totalBytes > 0ULL && usedBytes <= totalBytes;

    // Read-only growth telemetry for the append-oriented files whose scan cost
    // matters most. These values are observability only: no automatic cleanup,
    // truncation, compaction, or rotation is performed here.
    const uint64_t warehouseMovementsBytes = ready
        ? readOnlyFileSize(m_storage, "/data/warehouse/movements.ndjson") : 0ULL;
    const uint64_t windingEventsBytes = ready
        ? readOnlyFileSize(m_storage, "/data/winding-runs/events.ndjson") : 0ULL;
    const uint64_t repairRegistryBytes = ready
        ? readOnlyFileSize(m_storage, "/data/workshop/repairs.ndjson") : 0ULL;
    const uint64_t wireSpoolsBytes = ready
        ? readOnlyFileSize(m_storage, "/data/warehouse/spools.ndjson") : 0ULL;

    String response;
    response.reserve(430U);
    response = F("{\"storage_ready\":");
    response += ready ? F("true") : F("false");
    response += F(",\"card_size_bytes\":");
    appendUint64(response, cardSize);
    response += F(",\"filesystem_total_bytes\":");
    appendUint64(response, totalBytes);
    response += F(",\"filesystem_used_bytes\":");
    appendUint64(response, usedBytes);
    response += F(",\"filesystem_free_bytes\":");
    appendUint64(response, freeBytes);
    response += F(",\"warehouse_movements_bytes\":");
    appendUint64(response, warehouseMovementsBytes);
    response += F(",\"winding_events_bytes\":");
    appendUint64(response, windingEventsBytes);
    response += F(",\"repair_registry_bytes\":");
    appendUint64(response, repairRegistryBytes);
    response += F(",\"wire_spools_bytes\":");
    appendUint64(response, wireSpoolsBytes);
    response += F(",\"ndjson_growth_monitoring_only\":true");
    response += F(",\"automatic_cleanup_allowed\":false}");

    m_server.send(200, "application/json; charset=utf-8", response);
}
}