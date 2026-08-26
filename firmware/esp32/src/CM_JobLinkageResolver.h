#pragma once

#include <Arduino.h>
#include <FS.h>

#include "CM_JobSnapshotStore.h"
#include "CM_MotorWindingVersionStore.h"

namespace CM
{
class JobLinkageResolver
{
public:
    explicit JobLinkageResolver(fs::FS& storage);

    bool begin();
    bool isReady() const;

    // Resolves the immutable motor assignment stored on the same repair row.
    // Returns false for an unavailable store, an unknown/closed repair,
    // malformed data, duplicate repair identifiers or a mismatching motor.
    bool resolve(uint32_t repairId,
                 uint32_t requestedMotorId,
                 JobLinkage& linkage) const;

    // Backward-compatible WORKING lookup. New linked-job callers should pass
    // the requested role explicitly through the overload below.
    bool resolveWithProgram(uint32_t repairId,
                            uint32_t requestedMotorId,
                            JobLinkage& linkage,
                            String& coilProgram) const;

    // Resolves repair->motor linkage and the authoritative winding program for
    // the requested role. The latest winding version wins when present.
    // Legacy motor.coil_program remains a WORKING-only fallback. STARTING
    // without a versioned STARTING role fails closed.
    bool resolveWithProgram(uint32_t repairId,
                            uint32_t requestedMotorId,
                            RemoteJobType requestedType,
                            JobLinkage& linkage,
                            String& coilProgram) const;

    // Linked production path: resolves both the exact program and the repeat
    // target from the same authoritative winding role. Callers must compare
    // both values before committing immutable job state.
    bool resolveWithProgramAndRepeat(uint32_t repairId,
                                     uint32_t requestedMotorId,
                                     RemoteJobType requestedType,
                                     JobLinkage& linkage,
                                     String& coilProgram,
                                     uint16_t& repeatTarget) const;

private:
    static constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
    static constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
    static constexpr const char* RepairStatusPath = "/data/workshop/repair-status.ndjson";

    fs::FS& m_storage;
    MotorWindingVersionStore m_windingVersions;
    bool m_ready;
    bool m_windingVersionsReady;

    bool repairIsOpen(uint32_t repairId) const;
    bool resolveLegacyWorkingProgram(uint32_t requestedMotorId,
                                     JobLinkage& linkage,
                                     String& coilProgram,
                                     uint16_t& repeatTarget) const;
    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);
    static bool findString(const String& line,
                           const char* key,
                           String& value);
};
}
