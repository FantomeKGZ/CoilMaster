#ifndef CM_MOTOR_WINDING_VERSION_STORE_H
#define CM_MOTOR_WINDING_VERSION_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class WindingRole : uint8_t
{
    Working = 0U,
    Starting = 1U
};

struct WindingConductorSpec
{
    uint16_t diameterHundredthsMm;
    uint8_t strandCount;
    String materialClass;

    WindingConductorSpec()
        : diameterHundredthsMm(0U), strandCount(0U)
    {
    }
};

struct MotorWindingRoleSpec
{
    bool present;
    String coilProgram;
    uint16_t repeatTarget;
    uint16_t coilPitch;
    static constexpr uint8_t MaxConductors = 4U;
    WindingConductorSpec conductors[MaxConductors];
    uint8_t conductorCount;

    MotorWindingRoleSpec()
        : present(false), repeatTarget(1U), coilPitch(0U), conductorCount(0U)
    {
    }
};

struct NewMotorWindingVersion
{
    uint32_t motorId;
    uint32_t previousVersionId;
    uint32_t sourceRepairId;
    uint32_t sourceAutonomousSessionId;
    uint32_t sourceAutonomousRunId;
    String sourceAutonomousRole;
    String versionKind;
    String createdAt;
    String comment;
    MotorWindingRoleSpec working;
    MotorWindingRoleSpec starting;

    NewMotorWindingVersion()
        : motorId(0UL), previousVersionId(0UL), sourceRepairId(0UL),
          sourceAutonomousSessionId(0UL), sourceAutonomousRunId(0UL)
    {
    }
};

class MotorWindingVersionStore
{
public:
    static constexpr const char* Path = "/data/workshop/motor-winding-versions.ndjson";
    static constexpr uint8_t MaxPageSize = 24U;

    explicit MotorWindingVersionStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewMotorWindingVersion& version, uint32_t& versionId);
    bool loadLatestByMotor(uint32_t motorId,
                           NewMotorWindingVersion& version,
                           uint32_t& versionId,
                           bool& found) const;
    bool findAutonomousProjection(uint32_t sessionId,
                                  uint32_t runId,
                                  const String& role,
                                  uint32_t& motorId,
                                  uint32_t& versionId,
                                  bool& found) const;
    bool appendLatestByMotorJson(String& json, uint32_t motorId, bool& found) const;
    bool appendByVersionIdJson(String& json,
                               uint32_t versionId,
                               uint32_t expectedMotorId,
                               bool& found) const;
    bool appendMotorPageJson(String& json,
                             uint32_t motorId,
                             uint32_t cursor,
                             uint8_t limit,
                             uint16_t& count,
                             uint32_t& nextCursor,
                             bool& hasMore) const;

    static bool canonicalConductors(const MotorWindingRoleSpec& role,
                                    String& canonical);

private:
    bool ensureDirectory();
    bool nextVersionId(uint32_t& versionId) const;
    static bool validRole(const MotorWindingRoleSpec& role);
    static bool validMaterialClass(const String& value);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findOptionalUnsigned(const String& line,
                                     const char* key,
                                     uint32_t& value,
                                     bool& present);
    static bool findString(const String& line,
                           const char* key,
                           String& value,
                           bool required);
    static bool findBool(const String& line, const char* key, bool& value);
    static bool parseConductors(const String& canonical,
                                MotorWindingRoleSpec& role);
    static bool parseRole(const String& line,
                          const char* prefix,
                          bool present,
                          MotorWindingRoleSpec& role);
    static bool parseVersion(const String& line,
                             NewMotorWindingVersion& version,
                             uint32_t& versionId);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
