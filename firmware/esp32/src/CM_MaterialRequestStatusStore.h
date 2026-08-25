#ifndef CM_MATERIAL_REQUEST_STATUS_STORE_H
#define CM_MATERIAL_REQUEST_STATUS_STORE_H

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

namespace CM
{
struct MaterialRequestStatusState
{
    uint32_t materialRequestId;
    String status;
    uint32_t transitionCount;

    MaterialRequestStatusState()
        : materialRequestId(0UL), status("DRAFT"), transitionCount(0UL)
    {
    }
};

class MaterialRequestStatusStore
{
public:
    static constexpr const char* Path = "/data/workshop/material-request-status.ndjson";

    explicit MaterialRequestStatusStore(fs::FS& storage);

    bool begin();
    bool ready() const;

    // Successful lookup of a missing request returns true with found=false;
    // storage/validation failures return false so Web can distinguish 404 from 5xx.
    bool resolve(uint32_t materialRequestId,
                 MaterialRequestStatusState& state,
                 bool& found) const;

    bool transition(uint32_t materialRequestId,
                    const String& targetStatus,
                    const String& changedAt,
                    uint32_t& transitionId);

    static bool validStatus(const String& status);
    static bool validTransition(const String& fromStatus,
                                const String& toStatus);

private:
    static constexpr const char* RequestsPath =
        "/data/workshop/material-requests.ndjson";

    bool ensureDirectory();
    bool requestExists(uint32_t materialRequestId, bool& found) const;
    bool nextTransitionId(uint32_t& transitionId) const;
    bool validateStatusFileStructure() const;

    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);
    static bool findString(const String& line,
                           const char* key,
                           String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_MATERIAL_REQUEST_STATUS_STORE_H
