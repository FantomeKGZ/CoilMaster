#include "CM_RepairAsReceivedSnapshotStore.h"

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

RepairAsReceivedSnapshotStore::RepairAsReceivedSnapshotStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool RepairAsReceivedSnapshotStore::begin()
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

bool RepairAsReceivedSnapshotStore::ready() const
{
    return m_ready;
}

bool RepairAsReceivedSnapshotStore::append(const NewRepairAsReceivedSnapshot& snapshot,
                                           uint32_t& snapshotId)
{
    snapshotId = 0UL;
    if (!ready() || snapshot.repairId == 0UL || snapshot.clientId == 0UL ||
        snapshot.motorId == 0UL || snapshot.capturedAt.length() < 10U ||
        snapshot.sourceKind.length() == 0U || snapshot.motorName.length() == 0U ||
        snapshot.workingRepeatTarget == 0U ||
        !WindingProgramParser::valid(snapshot.workingProgram) ||
        (snapshot.startingPresent &&
         (snapshot.startingRepeatTarget == 0U ||
          !WindingProgramParser::valid(snapshot.startingProgram))) ||
        (!snapshot.startingPresent &&
         (snapshot.startingProgram.length() > 0U || snapshot.startingConductors.length() > 0U)) ||
        !nextSnapshotId(snapshotId))
    {
        return false;
    }

    String workingProgram;
    String startingProgram;
    if (!WindingProgramParser::canonicalize(snapshot.workingProgram, workingProgram))
        return false;
    if (snapshot.startingPresent &&
        !WindingProgramParser::canonicalize(snapshot.startingProgram, startingProgram))
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
    line.reserve(1280U);
    line = F("{\"snapshot_id\":"); line += snapshotId;
    line += F(",\"repair_id\":"); line += snapshot.repairId;
    line += F(",\"client_id\":"); line += snapshot.clientId;
    line += F(",\"motor_id\":"); line += snapshot.motorId;
    if (snapshot.windingVersionId > 0UL)
    {
        line += F(",\"winding_version_id\":"); line += snapshot.windingVersionId;
    }
    line += F(",\"captured_at\":\""); line += jsonEscape(snapshot.capturedAt);
    line += F("\",\"source_kind\":\""); line += jsonEscape(snapshot.sourceKind);
    line += F("\",\"motor_name\":\""); line += jsonEscape(snapshot.motorName);
    line += F("\",\"manufacturer\":\""); line += jsonEscape(snapshot.manufacturer);
    line += F("\",\"model\":\""); line += jsonEscape(snapshot.model);
    line += '"';
    if (snapshot.phases > 0U)
    {
        line += F(",\"phases\":"); line += snapshot.phases;
    }
    if (snapshot.slotCount > 0U)
    {
        line += F(",\"slot_count\":"); line += snapshot.slotCount;
    }
    line += F(",\"working_program\":\""); line += workingProgram;
    line += F("\",\"working_repeat_target\":"); line += snapshot.workingRepeatTarget;
    if (snapshot.workingConductors.length() > 0U)
    {
        line += F(",\"working_conductors\":\"");
        line += jsonEscape(snapshot.workingConductors); line += '"';
    }
    line += F(",\"starting_present\":");
    line += snapshot.startingPresent ? F("true") : F("false");
    if (snapshot.startingPresent)
    {
        line += F(",\"starting_program\":\""); line += startingProgram;
        line += F("\",\"starting_repeat_target\":"); line += snapshot.startingRepeatTarget;
        if (snapshot.startingConductors.length() > 0U)
        {
            line += F(",\"starting_conductors\":\"");
            line += jsonEscape(snapshot.startingConductors); line += '"';
        }
    }
    if (snapshot.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(snapshot.comment); line += '"';
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

bool RepairAsReceivedSnapshotStore::appendByRepairIdJson(String& json,
                                                         uint32_t repairId,
                                                         bool& found) const
{
    found = false;
    if (!ready() || repairId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentRepairId = 0UL;
        if (!findUnsigned(line, "repair_id", currentRepairId) || currentRepairId == 0UL)
        {
            file.close();
            return false;
        }
        if (currentRepairId != repairId) continue;
        if (found)
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

bool RepairAsReceivedSnapshotStore::ensureDirectory()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists("/data/workshop") &&
        !m_storage.mkdir("/data/workshop"))
    {
        return false;
    }
    return true;
}

bool RepairAsReceivedSnapshotStore::nextSnapshotId(uint32_t& snapshotId) const
{
    snapshotId = 1UL;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousSnapshotId = 0UL;
    uint32_t previousRepairId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t currentSnapshotId = 0UL;
        uint32_t currentRepairId = 0UL;
        if (!findUnsigned(line, "snapshot_id", currentSnapshotId) || currentSnapshotId == 0UL ||
            !findUnsigned(line, "repair_id", currentRepairId) || currentRepairId == 0UL ||
            currentSnapshotId <= previousSnapshotId || currentRepairId <= previousRepairId)
        {
            file.close();
            return false;
        }
        previousSnapshotId = currentSnapshotId;
        previousRepairId = currentRepairId;
    }
    file.close();
    if (previousSnapshotId == 0xFFFFFFFFUL) return false;
    snapshotId = previousSnapshotId + 1UL;
    return true;
}

bool RepairAsReceivedSnapshotStore::findUnsigned(const String& line,
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

String RepairAsReceivedSnapshotStore::jsonEscape(const String& value)
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
