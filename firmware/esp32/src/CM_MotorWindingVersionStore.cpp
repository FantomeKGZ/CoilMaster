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
    const bool hasAutonomousSource = version.sourceAutonomousSessionId > 0UL ||
                                     version.sourceAutonomousRunId > 0UL ||
                                     version.sourceAutonomousRole.length() > 0U;
    const bool validAutonomousSource = !hasAutonomousSource ||
        (version.sourceAutonomousSessionId > 0UL &&
         version.sourceAutonomousRunId > 0UL &&
         (version.sourceAutonomousRole == "WORKING" ||
          version.sourceAutonomousRole == "STARTING"));
    if (!ready() || version.motorId == 0UL || version.versionKind.length() == 0U ||
        version.createdAt.length() < 10U || !validRole(version.working) ||
        !validRole(version.starting) || !version.working.present ||
        !validAutonomousSource || !nextVersionId(versionId))
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
    line.reserve(1080U);
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
    if (hasAutonomousSource)
    {
        line += F(",\"source_autonomous_session_id\":");
        line += version.sourceAutonomousSessionId;
        line += F(",\"source_autonomous_run_id\":");
        line += version.sourceAutonomousRunId;
        line += F(",\"source_autonomous_role\":\"");
        line += version.sourceAutonomousRole;
        line += '"';
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

bool MotorWindingVersionStore::loadLatestByMotor(uint32_t motorId,
                                                  NewMotorWindingVersion& version,
                                                  uint32_t& versionId,
                                                  bool& found) const
{
    version = NewMotorWindingVersion();
    versionId = 0UL;
    found = false;
    if (!ready() || motorId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentVersionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", currentVersionId) ||
            currentVersionId == 0UL || currentVersionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = currentVersionId;
        if (currentMotorId != motorId) continue;

        NewMotorWindingVersion parsed;
        uint32_t parsedVersionId = 0UL;
        if (!parseVersion(line, parsed, parsedVersionId) ||
            parsedVersionId != currentVersionId || parsed.motorId != motorId)
        {
            file.close();
            return false;
        }
        version = parsed;
        versionId = currentVersionId;
        found = true;
    }
    file.close();
    return true;
}

bool MotorWindingVersionStore::findAutonomousProjection(uint32_t sessionId,
                                                         uint32_t runId,
                                                         const String& role,
                                                         uint32_t& motorId,
                                                         uint32_t& versionId,
                                                         bool& found) const
{
    motorId = 0UL;
    versionId = 0UL;
    found = false;
    if (!ready() || sessionId == 0UL || runId == 0UL ||
        (role != "WORKING" && role != "STARTING"))
    {
        return false;
    }
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentVersionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", currentVersionId) ||
            currentVersionId == 0UL || currentVersionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = currentVersionId;

        uint32_t sourceSessionId = 0UL;
        uint32_t sourceRunId = 0UL;
        bool sessionPresent = false;
        bool runPresent = false;
        if (!findOptionalUnsigned(line, "source_autonomous_session_id",
                                  sourceSessionId, sessionPresent) ||
            !findOptionalUnsigned(line, "source_autonomous_run_id",
                                  sourceRunId, runPresent))
        {
            file.close();
            return false;
        }
        String sourceRole;
        const bool rolePresent = line.indexOf(F("\"source_autonomous_role\":")) >= 0;
        if (sessionPresent || runPresent || rolePresent)
        {
            if (!sessionPresent || !runPresent || !rolePresent ||
                sourceSessionId == 0UL || sourceRunId == 0UL ||
                !findString(line, "source_autonomous_role", sourceRole, true) ||
                (sourceRole != "WORKING" && sourceRole != "STARTING"))
            {
                file.close();
                return false;
            }
        }
        else
        {
            continue;
        }

        if (sourceSessionId == sessionId && sourceRunId == runId && sourceRole == role)
        {
            if (found)
            {
                file.close();
                return false;
            }
            found = true;
            motorId = currentMotorId;
            versionId = currentVersionId;
        }
    }
    file.close();
    return true;
}

bool MotorWindingVersionStore::appendLatestByMotorJson(String& json,
                                                       uint32_t motorId,
                                                       bool& found) const
{
    found = false;
    if (!ready() || motorId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    String latest;
    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentVersionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", currentVersionId) || currentVersionId == 0UL ||
            currentVersionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = currentVersionId;
        if (currentMotorId == motorId)
        {
            latest = line;
            found = true;
        }
    }
    file.close();
    if (found) json += latest;
    return true;
}

bool MotorWindingVersionStore::appendByVersionIdJson(String& json,
                                                     uint32_t versionId,
                                                     uint32_t expectedMotorId,
                                                     bool& found) const
{
    found = false;
    if (!ready() || versionId == 0UL || expectedMotorId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentVersionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", currentVersionId) ||
            currentVersionId == 0UL || currentVersionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = currentVersionId;
        if (currentVersionId != versionId) continue;
        if (currentMotorId != expectedMotorId || found)
        {
            file.close();
            return false;
        }
        json += line;
        found = true;
    }
    file.close();
    return true;
}

bool MotorWindingVersionStore::appendMotorPageJson(String& json,
                                                   uint32_t motorId,
                                                   uint32_t cursor,
                                                   uint8_t limit,
                                                   uint16_t& count,
                                                   uint32_t& nextCursor,
                                                   bool& hasMore) const
{
    count = 0U;
    nextCursor = 0UL;
    hasMore = false;
    if (!ready() || motorId == 0UL || limit == 0U || limit > MaxPageSize)
        return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousVersionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t versionId = 0UL;
        uint32_t currentMotorId = 0UL;
        if (!findUnsigned(line, "winding_version_id", versionId) || versionId == 0UL ||
            versionId <= previousVersionId ||
            !findUnsigned(line, "motor_id", currentMotorId) || currentMotorId == 0UL)
        {
            file.close();
            return false;
        }
        previousVersionId = versionId;
        if (versionId <= cursor || currentMotorId != motorId) continue;
        if (count >= limit)
        {
            hasMore = true;
            break;
        }
        if (count > 0U) json += ',';
        json += line;
        ++count;
        nextCursor = versionId;
    }
    file.close();
    if (!hasMore) nextCursor = 0UL;
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

bool MotorWindingVersionStore::findOptionalUnsigned(const String& line,
                                                    const char* key,
                                                    uint32_t& value,
                                                    bool& present)
{
    value = 0UL;
    String marker = F("\""); marker += key; marker += F("\":");
    present = line.indexOf(marker) >= 0;
    if (!present) return true;
    return findUnsigned(line, key, value);
}

bool MotorWindingVersionStore::findString(const String& line,
                                           const char* key,
                                           String& value,
                                           bool required)
{
    value = String();
    String marker = F("\""); marker += key; marker += F("\":\"");
    const int index = line.indexOf(marker);
    if (index < 0) return !required;
    size_t pos = static_cast<size_t>(index) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (escaped)
        {
            if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else if (ch == '\\' || ch == '"') value += ch;
            else return false;
            escaped = false;
            continue;
        }
        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"') return true;
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool MotorWindingVersionStore::findBool(const String& line,
                                         const char* key,
                                         bool& value)
{
    String marker = F("\""); marker += key; marker += F("\":");
    const int index = line.indexOf(marker);
    if (index < 0) return false;
    const size_t pos = static_cast<size_t>(index) + marker.length();
    if (line.substring(pos, pos + 4U) == "true")
    {
        value = true;
        return true;
    }
    if (line.substring(pos, pos + 5U) == "false")
    {
        value = false;
        return true;
    }
    return false;
}

bool MotorWindingVersionStore::parseConductors(const String& canonical,
                                                MotorWindingRoleSpec& role)
{
    role.conductorCount = 0U;
    if (canonical.length() == 0U) return true;

    size_t start = 0U;
    while (start < canonical.length())
    {
        if (role.conductorCount >= MotorWindingRoleSpec::MaxConductors) return false;
        int plus = canonical.indexOf('+', start);
        const size_t end = plus < 0 ? canonical.length() : static_cast<size_t>(plus);
        if (end <= start) return false;
        const String token = canonical.substring(start, end);
        const int colon = token.indexOf(':');
        const int x = token.indexOf('x', colon + 1);
        if (colon <= 0 || x <= colon + 1 || static_cast<size_t>(x + 1) >= token.length())
            return false;

        WindingConductorSpec& conductor = role.conductors[role.conductorCount];
        conductor.materialClass = token.substring(0U, static_cast<size_t>(colon));
        if (!validMaterialClass(conductor.materialClass)) return false;

        uint32_t diameter = 0UL;
        for (int i = colon + 1; i < x; ++i)
        {
            if (!isDigit(token[static_cast<size_t>(i)])) return false;
            diameter = diameter * 10UL + static_cast<uint8_t>(token[static_cast<size_t>(i)] - '0');
            if (diameter > 0xFFFFU) return false;
        }
        uint32_t strands = 0UL;
        for (size_t i = static_cast<size_t>(x + 1); i < token.length(); ++i)
        {
            if (!isDigit(token[i])) return false;
            strands = strands * 10UL + static_cast<uint8_t>(token[i] - '0');
            if (strands > 0xFFU) return false;
        }
        if (diameter == 0UL || strands == 0UL) return false;
        conductor.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        conductor.strandCount = static_cast<uint8_t>(strands);
        ++role.conductorCount;
        if (plus < 0) break;
        start = end + 1U;
    }
    return true;
}

bool MotorWindingVersionStore::parseRole(const String& line,
                                          const char* prefix,
                                          bool present,
                                          MotorWindingRoleSpec& role)
{
    role = MotorWindingRoleSpec();
    role.present = present;
    if (!present) return true;

    String key = prefix; key += F("_program");
    if (!findString(line, key.c_str(), role.coilProgram, true)) return false;

    uint32_t repeatTarget = 0UL;
    key = prefix; key += F("_repeat_target");
    if (!findUnsigned(line, key.c_str(), repeatTarget) ||
        repeatTarget == 0UL || repeatTarget > 0xFFFFU)
    {
        return false;
    }
    role.repeatTarget = static_cast<uint16_t>(repeatTarget);

    uint32_t coilPitch = 0UL;
    bool coilPitchPresent = false;
    key = prefix; key += F("_coil_pitch");
    if (!findOptionalUnsigned(line, key.c_str(), coilPitch, coilPitchPresent) ||
        coilPitch > 0xFFFFU)
    {
        return false;
    }
    role.coilPitch = static_cast<uint16_t>(coilPitch);

    String conductors;
    key = prefix; key += F("_conductors");
    if (!findString(line, key.c_str(), conductors, false) ||
        !parseConductors(conductors, role))
    {
        return false;
    }
    return validRole(role);
}

bool MotorWindingVersionStore::parseVersion(const String& line,
                                             NewMotorWindingVersion& version,
                                             uint32_t& versionId)
{
    version = NewMotorWindingVersion();
    versionId = 0UL;
    if (!findUnsigned(line, "winding_version_id", versionId) || versionId == 0UL ||
        !findUnsigned(line, "motor_id", version.motorId) || version.motorId == 0UL)
    {
        return false;
    }

    bool present = false;
    if (!findOptionalUnsigned(line, "previous_version_id", version.previousVersionId, present))
        return false;
    if (!findOptionalUnsigned(line, "source_repair_id", version.sourceRepairId, present))
        return false;

    bool sourceSessionPresent = false;
    bool sourceRunPresent = false;
    if (!findOptionalUnsigned(line, "source_autonomous_session_id",
                              version.sourceAutonomousSessionId, sourceSessionPresent) ||
        !findOptionalUnsigned(line, "source_autonomous_run_id",
                              version.sourceAutonomousRunId, sourceRunPresent))
    {
        return false;
    }
    const bool sourceRolePresent = line.indexOf(F("\"source_autonomous_role\":")) >= 0;
    if (sourceSessionPresent || sourceRunPresent || sourceRolePresent)
    {
        if (!sourceSessionPresent || !sourceRunPresent || !sourceRolePresent ||
            version.sourceAutonomousSessionId == 0UL ||
            version.sourceAutonomousRunId == 0UL ||
            !findString(line, "source_autonomous_role", version.sourceAutonomousRole, true) ||
            (version.sourceAutonomousRole != "WORKING" &&
             version.sourceAutonomousRole != "STARTING"))
        {
            return false;
        }
    }

    if (!findString(line, "version_kind", version.versionKind, true) ||
        !findString(line, "created_at", version.createdAt, true) ||
        version.versionKind.length() == 0U || version.createdAt.length() < 10U)
    {
        return false;
    }
    findString(line, "comment", version.comment, false);

    bool startingPresent = false;
    if (!findBool(line, "starting_present", startingPresent) ||
        !parseRole(line, "working", true, version.working) ||
        !parseRole(line, "starting", startingPresent, version.starting))
    {
        return false;
    }
    return version.working.present;
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
