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
    String versionKind;
    String createdAt;
    String comment;
    MotorWindingRoleSpec working;
    MotorWindingRoleSpec starting;

    NewMotorWindingVersion()
        : motorId(0UL), previousVersionId(0UL), sourceRepairId(0UL)
    {
    }
};

class MotorWindingVersionStore
{
public:
    static constexpr const char* Path = "/data/workshop/motor-winding-versions.ndjson";

    explicit MotorWindingVersionStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewMotorWindingVersion& version, uint32_t& versionId);

    static bool canonicalConductors(const MotorWindingRoleSpec& role,
                                    String& canonical);

private:
    bool ensureDirectory();
    bool nextVersionId(uint32_t& versionId) const;
    static bool validRole(const MotorWindingRoleSpec& role);
    static bool validMaterialClass(const String& value);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
