#ifndef CM_MATERIAL_REQUEST_STORE_H
#define CM_MATERIAL_REQUEST_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NewMaterialRequest
{
    uint32_t repairId;
    uint32_t clientId;
    uint32_t motorId;
    String createdAt;
    String comment;

    NewMaterialRequest()
        : repairId(0UL), clientId(0UL), motorId(0UL)
    {
    }
};

class MaterialRequestStore
{
public:
    static constexpr const char* Path = "/data/workshop/material-requests.ndjson";
    static constexpr uint8_t MaxPageSize = 24U;

    explicit MaterialRequestStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewMaterialRequest& request, uint32_t& requestId);
    bool appendByIdJson(String& json, uint32_t requestId, bool& found) const;
    bool appendRepairPageJson(String& json,
                              uint32_t repairId,
                              uint32_t cursor,
                              uint8_t limit,
                              uint16_t& count,
                              uint32_t& nextCursor,
                              bool& hasMore) const;

private:
    bool ensureDirectory();
    bool nextRequestId(uint32_t& requestId) const;
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_MATERIAL_REQUEST_STORE_H
