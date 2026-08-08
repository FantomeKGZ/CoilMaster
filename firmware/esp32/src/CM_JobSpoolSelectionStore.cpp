#include "CM_JobSpoolSelectionStore.h"
#include "CM_FlatJsonObjectValidator.h"
#include <string.h>

namespace CM
{
namespace
{
String baseNameOf(const String& path)
{
    const int slash = path.lastIndexOf('/');
    return slash >= 0 ? path.substring(slash + 1) : path;
}

bool canonicalSessionFileName(const String& name,
                              const char* suffix,
                              uint32_t& sessionId)
{
    sessionId = 0UL;
    const String base = baseNameOf(name);
    if (!base.startsWith(F("session-")) || !base.endsWith(suffix)) return false;
    const size_t prefixLength = 8U;
    const size_t suffixLength = strlen(suffix);
    if (base.length() <= prefixLength + suffixLength) return false;
    const String digits = base.substring(prefixLength, base.length() - suffixLength);
    if (digits.length() == 0U || (digits.length() > 1U && digits[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < digits.length(); ++i)
    {
        if (!isDigit(digits[i])) return false;
        const uint8_t digit = static_cast<uint8_t>(digits[i] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    if (parsed == 0UL) return false;
    sessionId = parsed;
    return true;
}
}

JobSpoolSelectionStore::JobSpoolSelectionStore(fs::FS& storage)
    : m_storage(storage), m_ready(false) {}

bool JobSpoolSelectionStore::begin()
{
    m_ready = false;
    if (!ensureDirectories()) return false;

    const auto auditDirectory = [this](bool allowRecoverableTemp,
                                       uint32_t& recoverableSessionId) -> bool
    {
        recoverableSessionId = 0UL;
        File directory = m_storage.open(SelectionDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return false;
        }

        File entry = directory.openNextFile();
        while (entry)
        {
            if (entry.isDirectory())
            {
                entry.close();
                directory.close();
                return false;
            }

            const String base = baseNameOf(entry.name());
            const size_t size = entry.size();
            if (size == 0U || size >= 512U)
            {
                entry.close();
                directory.close();
                return false;
            }
            const String content = entry.readString();
            entry.close();

            JobSpoolSelection selection;
            if (!parse(content, selection))
            {
                directory.close();
                return false;
            }

            uint32_t fileSessionId = 0UL;
            if (base.endsWith(F(".json.tmp")))
            {
                if (!allowRecoverableTemp || recoverableSessionId != 0UL ||
                    !canonicalSessionFileName(base, ".json.tmp", fileSessionId) ||
                    fileSessionId != selection.sessionId ||
                    m_storage.exists(selectionPath(selection.sessionId)))
                {
                    directory.close();
                    return false;
                }
                recoverableSessionId = selection.sessionId;
            }
            else if (!canonicalSessionFileName(base, ".json", fileSessionId) ||
                     fileSessionId != selection.sessionId)
            {
                directory.close();
                return false;
            }

            entry = directory.openNextFile();
        }
        directory.close();
        return true;
    };

    uint32_t recoverableSessionId = 0UL;
    if (!auditDirectory(true, recoverableSessionId)) return false;

    if (recoverableSessionId != 0UL)
    {
        const String tempPath = temporaryPath(recoverableSessionId);
        const String finalPath = selectionPath(recoverableSessionId);
        if (!m_storage.rename(tempPath, finalPath)) return false;

        uint32_t unexpectedTemp = 0UL;
        if (!auditDirectory(false, unexpectedTemp) || unexpectedTemp != 0UL)
            return false;
    }

    m_ready = true;
    return true;
}

bool JobSpoolSelectionStore::isReady() const
{
    if (!m_ready) return false;
    File directory = m_storage.open(SelectionDirectory, FILE_READ);
    if (!directory) return false;
    const bool ready = directory.isDirectory();
    directory.close();
    return ready;
}

bool JobSpoolSelectionStore::create(const JobSpoolSelection& selection)
{
    if (!isReady() || !selection.isValid()) return false;
    const String finalPath = selectionPath(selection.sessionId);
    const String tempPath = temporaryPath(selection.sessionId);
    if (m_storage.exists(finalPath) || m_storage.exists(tempPath)) return false;

    const auto cleanupTemp = [this, &tempPath]()
    {
        if (m_storage.exists(tempPath) && !m_storage.remove(tempPath))
            m_ready = false;
    };

    String output;
    if (!serialize(selection, output)) return false;
    File file = m_storage.open(tempPath, FILE_WRITE);
    if (!file) return false;
    const size_t written = file.print(output);
    file.flush();
    file.close();
    if (written != output.length())
    {
        cleanupTemp();
        return false;
    }

    File verify = m_storage.open(tempPath, FILE_READ);
    if (!verify || verify.isDirectory())
    {
        if (verify) verify.close();
        cleanupTemp();
        return false;
    }
    const String verifiedText = verify.readString();
    verify.close();
    JobSpoolSelection verified;
    if (!parse(verifiedText, verified) ||
        verified.jobId != selection.jobId ||
        verified.sessionId != selection.sessionId ||
        verified.repairId != selection.repairId ||
        verified.motorId != selection.motorId ||
        verified.spoolId != selection.spoolId ||
        verified.diameterHundredthsMm != selection.diameterHundredthsMm ||
        verified.weightAtSelectionGrams != selection.weightAtSelectionGrams ||
        verified.wireType != selection.wireType)
    {
        cleanupTemp();
        return false;
    }

    if (!m_storage.rename(tempPath, finalPath))
    {
        cleanupTemp();
        return false;
    }
    return true;
}

bool JobSpoolSelectionStore::load(uint32_t sessionId,
                                  JobSpoolSelection& selection) const
{
    bool found = false;
    return load(sessionId, selection, found) && found;
}

bool JobSpoolSelectionStore::validateIdentity(uint32_t jobId,
                                              uint32_t sessionId) const
{
    JobSpoolSelection selection;
    return jobId != 0UL && load(sessionId, selection) && selection.jobId == jobId;
}

bool JobSpoolSelectionStore::ensureDirectories()
{
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists(RootDirectory) && !m_storage.mkdir(RootDirectory)) return false;
    if (!m_storage.exists(SelectionDirectory) &&
        !m_storage.mkdir(SelectionDirectory)) return false;
    return true;
}

String JobSpoolSelectionStore::selectionPath(uint32_t sessionId) const
{
    String path = F("/data/winding-jobs/spool-selection/session-");
    path += sessionId;
    path += F(".json");
    return path;
}

String JobSpoolSelectionStore::temporaryPath(uint32_t sessionId) const
{
    String path = selectionPath(sessionId);
    path += F(".tmp");
    return path;
}

bool JobSpoolSelectionStore::serialize(const JobSpoolSelection& selection,
                                       String& output)
{
    output = String();
    if (!selection.isValid()) return false;
    output.reserve(320U);
    output = F("{\"schema_version\":1,\"job_id\":"); output += selection.jobId;
    output += F(",\"session_id\":"); output += selection.sessionId;
    output += F(",\"repair_id\":"); output += selection.repairId;
    output += F(",\"motor_id\":"); output += selection.motorId;
    output += F(",\"spool_id\":"); output += selection.spoolId;
    output += F(",\"diameter_hundredths_mm\":"); output += selection.diameterHundredthsMm;
    output += F(",\"weight_at_selection_g\":"); output += selection.weightAtSelectionGrams;
    output += F(",\"wire_type\":\""); output += selection.wireType;
    output += F("\",\"automatic_writeoff_allowed\":false}\n");
    return output.length() < 512U;
}

bool JobSpoolSelectionStore::parse(const String& input,
                                   JobSpoolSelection& selection)
{
    selection = JobSpoolSelection();
    if (!FlatJsonObjectValidator::valid(input) ||
        !input.startsWith(F("{\"schema_version\":1,")) ||
        !input.endsWith(F("}\n")) || input.length() >= 512U)
    {
        return false;
    }

    uint32_t schemaVersion = 0UL;
    uint32_t diameter = 0UL;
    if (!findUnsigned(input, "schema_version", schemaVersion) || schemaVersion != 1UL ||
        !findUnsigned(input, "job_id", selection.jobId) || selection.jobId == 0UL ||
        !findUnsigned(input, "session_id", selection.sessionId) || selection.sessionId == 0UL ||
        !findUnsigned(input, "repair_id", selection.repairId) || selection.repairId == 0UL ||
        !findUnsigned(input, "motor_id", selection.motorId) || selection.motorId == 0UL ||
        !findUnsigned(input, "spool_id", selection.spoolId) || selection.spoolId == 0UL ||
        !findUnsigned(input, "diameter_hundredths_mm", diameter) ||
        diameter == 0UL || diameter > 0xFFFFUL ||
        !findUnsigned(input, "weight_at_selection_g", selection.weightAtSelectionGrams) ||
        selection.weightAtSelectionGrams == 0UL ||
        !findString(input, "wire_type", selection.wireType) ||
        (selection.wireType != "CU" && selection.wireType != "AL"))
    {
        return false;
    }

    const String marker = F("\"automatic_writeoff_allowed\":false");
    if (input.indexOf(marker) < 0 ||
        input.indexOf(marker, input.indexOf(marker) + marker.length()) >= 0)
    {
        return false;
    }

    selection.diameterHundredthsMm = static_cast<uint16_t>(diameter);
    return selection.isValid();
}

bool JobSpoolSelectionStore::findUnsigned(const String& input,
                                          const char* key,
                                          uint32_t& value)
{
    value = 0UL;
    const String marker = String("\"") + key + F("\":");
    const int position = input.indexOf(marker);
    if (position < 0 || input.indexOf(marker, position + marker.length()) >= 0)
        return false;
    int cursor = position + marker.length();
    if (cursor >= input.length() || !isDigit(input[cursor])) return false;
    if (input[cursor] == '0' && cursor + 1 < input.length() &&
        isDigit(input[cursor + 1])) return false;
    uint32_t parsed = 0UL;
    while (cursor < input.length() && isDigit(input[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(input[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= input.length() ||
        (input[cursor] != ',' && input[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool JobSpoolSelectionStore::findString(const String& input,
                                        const char* key,
                                        String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int position = input.indexOf(marker);
    if (position < 0 || input.indexOf(marker, position + marker.length()) >= 0)
        return false;
    int cursor = position + marker.length();
    while (cursor < input.length())
    {
        const char ch = input[cursor++];
        if (ch == '"')
            return cursor < input.length() &&
                   (input[cursor] == ',' || input[cursor] == '}');
        if (ch == '\\' || static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}
}
