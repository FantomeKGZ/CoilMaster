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

    String response;
    response.reserve(240U);
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
    response += F(",\"automatic_cleanup_allowed\":false}");

    m_server.send(200, "application/json; charset=utf-8", response);
}
}
