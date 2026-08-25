#include "CM_MotorWindingVersionStore.h"

#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}
}

MotorWindingVersionStore::MotorWindingVersionStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool MotorWindingVersionStore::begin()
{
    m_ready = false;
    if (!ensureDirectory()) return false;
    if (m_storage.exists(Path))
    {
        File file = m_storage.open(Path, FILE_READ);
        if (!prepareNdjson(file))
        {
            if (file) file.close();
            return false;
        }
        file.close();
    }
    m_ready = true;
    return true;
}

bool MotorWindingVersionStore::ready() const
{
    return m_ready;
}

bool MotorWindingVersionStore::append(const NewMotorWindingVersion& version,
                                      uint32_t& versionId)
{
    versionId = 0UL;
    if (!ready() || version.motorId == 0UL || version.versionKind.length() == 0U ||
        version.createdAt.length() < 10U || !validRole(version.working) ||
        !validRole(version.starting) || !version.working.present ||
        !nextVersionId(versionId))
    {
        return false;
    }

    String workingProgram;
    if (!CM::WindingProgramParser::canonicalize(version.working.coilProgram,
                                                 workingProgram))
    {
        return false;
    }

    String startingProgram;
    if (version.starting.present &&
        !CM::WindingProgramParser::canonicalize(version.starting.coilProgram,
                                                 startingProgram))
    {
        return false;
    }

    String workingConductors;
    String startingConductors;
    if (!canonicalConductors(version.working, workingConductors) ||
        !canonicalConductors(version.starting, startingConductors))
    {
        return false;
    }

    File file = m_storage.open(Path, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(960U);
    line = F("{\"winding_version_id\":"); line += versionId;
    line += F(",\"motor_id\":"); line += version.motorId;
    if (version.previousVersionId > 0UL)
    {
        line += F(",\"previous_version_id\":"); line += version.previousVersionId;
    }
    if (version.sourceRepairId > 0UL)
    {
        line += F(",\"source_repair_id\":"); line += version.sourceRepairId;
    }
    line += F(",\"version_kind\":\""); line += jsonEscape(version.versionKind);
    line += F("\",\"created_at\":\""); line += jsonEscape(version.createdAt);
    line += F("\",\"working_program\":\""); line += workingProgram;
    line += F("\",\"working_repeat_target\":"); line += version.working.repeatTarget;
    if (version.working.coilPitch > 0U)
    {
        line += F(",\"working_coil_pitch\":"); line += version.working.coilPitch;
    }
    if (workingConductors.length() > 0U)
    {
        line += F(",\"working_conductors\":\""); line += workingConductors; line += '"';
    }
    line += F(",\"starting_present\":");
    line += version.starting.present ? F("true") : F("false");
    if (version.starting.present)
    {
        line += F(",\"starting_program\":\""); line += startingProgram;
        line += F("\",\"starting_repeat_target\":"); line += version.starting.repeatTarget;
        if (version.starting.coilPitch > 0U)
        {
            line += F(",\"starting_coil_pitch\":"); line += version.starting.coilPitch;
        }
        if (startingConductors.length() > 0U)
        {
            line += F(",\"starting_conductors\":\""); line += startingConductors; line += '"';
        }
    }
    if (version.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(version.comment); line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool MotorWindingVersionStore::canonicalConductors(const MotorWindingRoleSpec& role,
                                                    String& canonical)
{
    canonical = String();
    if (!role.present) return role.conductorCount == 0U;
    if (role.conductorCount > MotorWindingRoleSpec::MaxConductors) return false;

    for (uint8_t i = 0U; i < role.conductorCount; ++i)
    {
        const WindingConductorSpec& conductor = role.conductors[i];
        if (conductor.diameterHundredthsMm == 0U || conductor.strandCount == 0U ||
            !validMaterialClass(conductor.materialClass))
        {
            canonical = String();
            return false;
        }
        if (i > 0U) canonical += '+';
        canonical += conductor.materialClass;
        canonical += ':';
        canonical += conductor.diameterHundredthsMm;
        canonical += 'x';
        canonical += conductor.strandCount;
    }
    return true;
}

bool MotorWindingVersionStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool MotorWindingVersionStore::nextVersionId(uint32_t& versionId) const
{
    versionId = 1UL;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t current = 0UL;
        if (!findUnsigned(line, "winding_version_id", current) || current == 0UL ||
            current <= previous)
        {
            file.close();
            return false;
        }
        previous = current;
    }
    file.close();
    if (previous == 0xFFFFFFFFUL) return false;
    versionId = previous + 1UL;
    return true;
}

bool MotorWindingVersionStore::validRole(const MotorWindingRoleSpec& role)
{
    if (!role.present)
        return role.coilProgram.length() == 0U && role.conductorCount == 0U;
    if (role.repeatTarget == 0U || role.conductorCount > MotorWindingRoleSpec::MaxConductors)
        return false;
    return WindingProgramParser::valid(role.coilProgram);
}

bool MotorWindingVersionStore::validMaterialClass(const String& value)
{
    return value == "CU" || value == "AL";
}

bool MotorWindingVersionStore::findUnsigned(const String& line,
                                            const char* key,
                                            uint32_t& value)
{
    value = 0UL;
    String marker = F("\""); marker += key; marker += F("\":");
    const int index = line.indexOf(marker);
    if (index < 0) return false;
    size_t pos = static_cast<size_t>(index) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    uint32_t parsed = 0UL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++pos;
    }
    value = parsed;
    return true;
}

String MotorWindingVersionStore::jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i)
    {
        const char ch = value[i];
        if (ch == '\\' || ch == '"')
        {
            escaped += '\\';
            escaped += ch;
        }
        else if (ch == '\n') escaped += F("\\n");
        else if (ch == '\r') escaped += F("\\r");
        else if (ch == '\t') escaped += F("\\t");
        else if (static_cast<uint8_t>(ch) >= 0x20U) escaped += ch;
    }
    return escaped;
}
}
