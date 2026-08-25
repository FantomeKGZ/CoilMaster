#include "CM_RepairIntakeCoordinator.h"

#include "CM_FlatJsonObjectValidator.h"

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

bool decodeJsonString(const String& source, int start, String& value, int& end)
{
    value = String();
    end = -1;
    bool escaped = false;
    for (int i = start; i < static_cast<int>(source.length()); ++i)
    {
        const char ch = source[i];
        if (!escaped && ch == '"')
        {
            end = i;
            return true;
        }
        if (!escaped && ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (escaped)
        {
            switch (ch)
            {
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                default: return false;
            }
            escaped = false;
        }
        else
        {
            value += ch;
        }
    }
    return false;
}
}

RepairIntakeCoordinator::RepairIntakeCoordinator(fs::FS& storage,
                                                 RepairRegistry& registry)
    : m_storage(storage),
      m_registry(registry),
      m_windingVersions(storage),
      m_snapshots(storage),
      m_pending(storage),
      m_ready(false)
{
}

bool RepairIntakeCoordinator::begin()
{
    m_ready = false;
    if (!m_registry.ready() || !m_windingVersions.begin() ||
        !m_snapshots.begin() || !m_pending.begin())
    {
        return false;
    }
    if (!recoverPending()) return false;
    m_ready = true;
    return true;
}

bool RepairIntakeCoordinator::ready() const
{
    return m_ready && m_registry.ready() && m_windingVersions.ready() &&
           m_snapshots.ready() && m_pending.ready();
}

RepairIntakeCreateResult RepairIntakeCoordinator::create(const NewRepair& repair,
                                                         uint32_t& repairId)
{
    repairId = 0UL;
    if (!ready()) return RepairIntakeCreateResult::Unavailable;
    if (m_pending.hasPending()) return RepairIntakeCreateResult::Busy;

    uint32_t expectedRepairId = 0UL;
    if (!nextExpectedRepairId(expectedRepairId))
        return RepairIntakeCreateResult::IntegrityFailed;

    RepairIntakePending pending;
    if (!preparePending(repair, expectedRepairId, pending))
        return RepairIntakeCreateResult::InvalidSource;
    if (!m_pending.save(pending))
        return RepairIntakeCreateResult::IntegrityFailed;

    uint32_t actualRepairId = 0UL;
    if (!m_registry.addRepair(repair, actualRepairId))
    {
        // If the registry is still readable and the expected id remains unused,
        // the append never committed and the prepared marker may be discarded.
        uint32_t nextId = 0UL;
        if (m_registry.ready() && nextExpectedRepairId(nextId) &&
            nextId == expectedRepairId && m_pending.clear())
        {
            return RepairIntakeCreateResult::RegistryRejected;
        }
        m_ready = false;
        return RepairIntakeCreateResult::IntegrityFailed;
    }
    if (actualRepairId != expectedRepairId)
    {
        m_ready = false;
        return RepairIntakeCreateResult::IntegrityFailed;
    }

    NewRepairAsReceivedSnapshot snapshot;
    if (!buildSnapshot(pending, snapshot))
    {
        m_ready = false;
        return RepairIntakeCreateResult::IntegrityFailed;
    }
    uint32_t snapshotId = 0UL;
    if (!m_snapshots.append(snapshot, snapshotId))
    {
        m_ready = false;
        return RepairIntakeCreateResult::IntegrityFailed;
    }

    bool snapshotFound = false;
    if (!snapshotMatchesPending(actualRepairId, pending, snapshotFound) ||
        !snapshotFound || !m_pending.clear())
    {
        m_ready = false;
        return RepairIntakeCreateResult::IntegrityFailed;
    }

    repairId = actualRepairId;
    return RepairIntakeCreateResult::Created;
}

bool RepairIntakeCoordinator::recoverPending()
{
    RepairIntakePending pending;
    bool pendingFound = false;
    if (!m_pending.load(pending, pendingFound)) return false;
    if (!pendingFound) return true;

    bool repairFound = false;
    if (!repairMatchesPending(pending.repairId, pending, repairFound)) return false;

    bool snapshotFound = false;
    if (!snapshotMatchesPending(pending.repairId, pending, snapshotFound)) return false;

    if (!repairFound)
    {
        if (snapshotFound) return false;
        uint32_t nextId = 0UL;
        if (!nextExpectedRepairId(nextId) || nextId != pending.repairId)
            return false;
        return m_pending.clear();
    }

    if (!snapshotFound)
    {
        NewRepairAsReceivedSnapshot snapshot;
        if (!buildSnapshot(pending, snapshot)) return false;
        uint32_t snapshotId = 0UL;
        if (!m_snapshots.append(snapshot, snapshotId)) return false;
        if (!snapshotMatchesPending(pending.repairId, pending, snapshotFound) ||
            !snapshotFound)
        {
            return false;
        }
    }

    return m_pending.clear();
}

bool RepairIntakeCoordinator::nextExpectedRepairId(uint32_t& repairId) const
{
    repairId = 1UL;
    if (!m_storage.exists(RepairsPath)) return true;

    File file = m_storage.open(RepairsPath, FILE_READ);
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
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", current) || current == 0UL ||
            current <= previous)
        {
            file.close();
            return false;
        }
        previous = current;
    }
    file.close();
    if (previous == 0xFFFFFFFFUL) return false;
    repairId = previous + 1UL;
    return true;
}

bool RepairIntakeCoordinator::preparePending(const NewRepair& repair,
                                             uint32_t repairId,
                                             RepairIntakePending& pending)
{
    pending = RepairIntakePending();
    if (repairId == 0UL || repair.clientId == 0UL || repair.motorId == 0UL ||
        repair.receivedAt.length() < 10U || !m_registry.clientExists(repair.clientId) ||
        !m_registry.motorExists(repair.motorId))
    {
        return false;
    }

    String latest;
    bool versionFound = false;
    if (!m_windingVersions.appendLatestByMotorJson(latest, repair.motorId,
                                                   versionFound))
    {
        return false;
    }

    uint32_t sourceVersionId = 0UL;
    if (versionFound &&
        (!findUnsigned(latest, "winding_version_id", sourceVersionId) ||
         sourceVersionId == 0UL))
    {
        return false;
    }

    pending.repairId = repairId;
    pending.clientId = repair.clientId;
    pending.motorId = repair.motorId;
    pending.sourceWindingVersionId = sourceVersionId;
    pending.receivedAt = repair.receivedAt;
    pending.sourceKind = versionFound ? F("VERSIONED") : F("LEGACY_MOTOR");
    return pending.valid();
}

bool RepairIntakeCoordinator::buildSnapshot(const RepairIntakePending& pending,
                                            NewRepairAsReceivedSnapshot& snapshot) const
{
    snapshot = NewRepairAsReceivedSnapshot();
    if (!pending.valid()) return false;

    String motorJson;
    bool motorFound = false;
    if (!m_registry.appendMotorByIdJson(motorJson, pending.motorId, motorFound) ||
        !motorFound)
    {
        return false;
    }

    uint32_t motorId = 0UL;
    if (!findUnsigned(motorJson, "motor_id", motorId) || motorId != pending.motorId ||
        !findString(motorJson, "name", snapshot.motorName) ||
        !findString(motorJson, "manufacturer", snapshot.manufacturer) ||
        !findString(motorJson, "model", snapshot.model))
    {
        return false;
    }

    uint32_t optional = 0UL;
    bool present = false;
    if (!findOptionalUnsigned(motorJson, "phases", optional, present) || optional > 255UL)
        return false;
    snapshot.phases = present ? static_cast<uint8_t>(optional) : 0U;
    if (!findOptionalUnsigned(motorJson, "slot_count", optional, present) ||
        optional > 0xFFFFUL)
    {
        return false;
    }
    snapshot.slotCount = present ? static_cast<uint16_t>(optional) : 0U;

    snapshot.repairId = pending.repairId;
    snapshot.clientId = pending.clientId;
    snapshot.motorId = pending.motorId;
    snapshot.windingVersionId = pending.sourceWindingVersionId;
    snapshot.capturedAt = pending.receivedAt;
    snapshot.sourceKind = pending.sourceKind;

    if (pending.sourceKind == "VERSIONED")
    {
        if (pending.sourceWindingVersionId == 0UL) return false;
        String versionJson;
        bool versionFound = false;
        if (!m_windingVersions.appendByVersionIdJson(versionJson,
                                                      pending.sourceWindingVersionId,
                                                      pending.motorId,
                                                      versionFound) ||
            !versionFound)
        {
            return false;
        }
        if (!findString(versionJson, "working_program", snapshot.workingProgram) ||
            !findUnsigned(versionJson, "working_repeat_target", optional) ||
            optional == 0UL || optional > 0xFFFFUL)
        {
            return false;
        }
        snapshot.workingRepeatTarget = static_cast<uint16_t>(optional);
        bool conductorsPresent = false;
        if (!findOptionalString(versionJson, "working_conductors",
                                snapshot.workingConductors, conductorsPresent))
        {
            return false;
        }
        if (!findBoolean(versionJson, "starting_present", snapshot.startingPresent))
            return false;
        if (snapshot.startingPresent)
        {
            if (!findString(versionJson, "starting_program", snapshot.startingProgram) ||
                !findUnsigned(versionJson, "starting_repeat_target", optional) ||
                optional == 0UL || optional > 0xFFFFUL)
            {
                return false;
            }
            snapshot.startingRepeatTarget = static_cast<uint16_t>(optional);
            if (!findOptionalString(versionJson, "starting_conductors",
                                    snapshot.startingConductors, conductorsPresent))
            {
                return false;
            }
        }
        return true;
    }

    if (pending.sourceKind != "LEGACY_MOTOR" ||
        pending.sourceWindingVersionId != 0UL ||
        !findString(motorJson, "coil_program", snapshot.workingProgram) ||
        !findUnsigned(motorJson, "repeat_target", optional) ||
        optional == 0UL || optional > 0xFFFFUL)
    {
        return false;
    }
    snapshot.workingRepeatTarget = static_cast<uint16_t>(optional);

    String material;
    bool materialPresent = false;
    uint32_t diameter = 0UL;
    uint32_t strands = 0UL;
    bool diameterPresent = false;
    bool strandsPresent = false;
    if (!findOptionalString(motorJson, "wire_material", material, materialPresent) ||
        !findOptionalUnsigned(motorJson, "wire_diameter_hundredths_mm",
                              diameter, diameterPresent) ||
        !findOptionalUnsigned(motorJson, "parallel_strands", strands, strandsPresent))
    {
        return false;
    }
    if (materialPresent || diameterPresent || strandsPresent)
    {
        if (!materialPresent || !diameterPresent || !strandsPresent ||
            (material != "CU" && material != "AL") ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            strands == 0UL || strands > 255UL)
        {
            return false;
        }
        snapshot.workingConductors = material;
        snapshot.workingConductors += ':';
        snapshot.workingConductors += diameter;
        snapshot.workingConductors += 'x';
        snapshot.workingConductors += strands;
    }
    snapshot.startingPresent = false;
    return true;
}

bool RepairIntakeCoordinator::repairMatchesPending(uint32_t repairId,
                                                   const RepairIntakePending& pending,
                                                   bool& found) const
{
    found = false;
    String repairJson;
    if (!m_registry.appendRepairByIdJson(repairJson, repairId, found)) return false;
    if (!found) return true;

    uint32_t clientId = 0UL, motorId = 0UL;
    String receivedAt;
    return findUnsigned(repairJson, "client_id", clientId) &&
           findUnsigned(repairJson, "motor_id", motorId) &&
           findString(repairJson, "received_at", receivedAt) &&
           clientId == pending.clientId && motorId == pending.motorId &&
           receivedAt == pending.receivedAt;
}

bool RepairIntakeCoordinator::snapshotMatchesPending(uint32_t repairId,
                                                     const RepairIntakePending& pending,
                                                     bool& found) const
{
    found = false;
    String snapshotJson;
    if (!m_snapshots.appendByRepairIdJson(snapshotJson, repairId, found)) return false;
    if (!found) return true;

    uint32_t clientId = 0UL, motorId = 0UL, versionId = 0UL;
    bool versionPresent = false;
    String capturedAt, sourceKind;
    if (!findUnsigned(snapshotJson, "client_id", clientId) ||
        !findUnsigned(snapshotJson, "motor_id", motorId) ||
        !findOptionalUnsigned(snapshotJson, "winding_version_id", versionId,
                              versionPresent) ||
        !findString(snapshotJson, "captured_at", capturedAt) ||
        !findString(snapshotJson, "source_kind", sourceKind))
    {
        return false;
    }
    const uint32_t normalizedVersionId = versionPresent ? versionId : 0UL;
    return clientId == pending.clientId && motorId == pending.motorId &&
           normalizedVersionId == pending.sourceWindingVersionId &&
           capturedAt == pending.receivedAt && sourceKind == pending.sourceKind;
}

bool RepairIntakeCoordinator::findUnsigned(const String& line,
                                           const char* key,
                                           uint32_t& value)
{
    bool present = false;
    return findOptionalUnsigned(line, key, value, present) && present;
}

bool RepairIntakeCoordinator::findOptionalUnsigned(const String& line,
                                                   const char* key,
                                                   uint32_t& value,
                                                   bool& present)
{
    value = 0UL;
    present = false;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0) return true;
    if (line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    uint32_t parsed = 0UL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    present = true;
    return true;
}

bool RepairIntakeCoordinator::findBoolean(const String& line,
                                          const char* key,
                                          bool& value)
{
    value = false;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    const int valueStart = start + marker.length();
    int valueEnd = valueStart;
    if (line.substring(valueStart, valueStart + 4) == "true")
    {
        value = true;
        valueEnd += 4;
    }
    else if (line.substring(valueStart, valueStart + 5) == "false")
    {
        valueEnd += 5;
    }
    else return false;
    return valueEnd < static_cast<int>(line.length()) &&
           (line[valueEnd] == ',' || line[valueEnd] == '}');
}

bool RepairIntakeCoordinator::findString(const String& line,
                                         const char* key,
                                         String& value)
{
    bool present = false;
    return findOptionalString(line, key, value, present) && present;
}

bool RepairIntakeCoordinator::findOptionalString(const String& line,
                                                 const char* key,
                                                 String& value,
                                                 bool& present)
{
    value = String();
    present = false;
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0) return true;
    if (line.indexOf(marker, start + marker.length()) >= 0) return false;
    const int valueStart = start + marker.length();
    int valueEnd = -1;
    if (!decodeJsonString(line, valueStart, value, valueEnd) ||
        valueEnd + 1 >= static_cast<int>(line.length()) ||
        (line[valueEnd + 1] != ',' && line[valueEnd + 1] != '}'))
    {
        return false;
    }
    present = true;
    return true;
}
}
