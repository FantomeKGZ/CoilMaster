#pragma once

#include <Arduino.h>
#include <FS.h>

#include "CM_JobSnapshotStore.h"

namespace CM
{
class JobLinkageResolver
{
public:
    explicit JobLinkageResolver(fs::FS& storage);

    bool begin();
    bool isReady() const;

    // Resolves the immutable motor assignment stored on the same repair row.
    // Returns false for an unavailable store, an unknown repair, malformed data,
    // duplicate repair identifiers or a mismatching requested motor identifier.
    bool resolve(uint32_t repairId,
                 uint32_t requestedMotorId,
                 JobLinkage& linkage) const;

    // Resolves the repair->motor linkage and the motor catalogue winding program
    // from the same persistent workshop dataset. Duplicate motor identifiers,
    // missing/inactive motors or an empty coil_program fail closed.
    bool resolveWithProgram(uint32_t repairId,
                            uint32_t requestedMotorId,
                            JobLinkage& linkage,
                            String& coilProgram) const;

private:
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    static constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";

    fs::FS& m_storage;
    bool m_ready;

    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);
    static bool findString(const String& line,
                           const char* key,
                           String& value);
};
}
