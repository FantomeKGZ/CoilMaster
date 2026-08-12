#include "CM_RepairRegistry.h"
#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
bool RepairRegistry::appendSimilarMotorsJson(String& json,
                                             const NewMotor& candidate,
                                             uint16_t& sameProgramCount,
                                             uint16_t& identityMatchCount,
                                             uint8_t& returnedCount,
                                             bool& itemsTruncated) const
{
    sameProgramCount = 0U;
    identityMatchCount = 0U;
    returnedCount = 0U;
    itemsTruncated = false;
    if (!ready()) return false;
    if (!WindingProgramParser::valid(candidate.coilProgram)) return false;
    if (!m_storage.exists(MotorsPath)) return true;

    String candidateName = candidate.name;
    String candidateModel = candidate.model;
    String candidateManufacturer = candidate.manufacturer;
    candidateName.trim(); candidateName.toLowerCase();
    candidateModel.trim(); candidateModel.toLowerCase();
    candidateManufacturer.trim(); candidateManufacturer.toLowerCase();

    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL ||
        (rawSize > 0U &&
         (!file.seek(static_cast<uint32_t>(rawSize - 1U)) ||
          file.read() != '\n' || !file.seek(0U))))
    {
        file.close();
        return false;
    }

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        String program;
        String name;
        String model;
        String manufacturer;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findString(line, "coil_program", program) ||
            !findString(line, "name", name) ||
            !findString(line, "model", model) ||
            !findString(line, "manufacturer", manufacturer) ||
            !WindingProgramParser::valid(program))
        {
            file.close();
            return false;
        }
        if (!WindingProgramParser::equivalent(program, candidate.coilProgram))
            continue;
        if (sameProgramCount == 0xFFFFU)
        {
            file.close();
            return false;
        }
        ++sameProgramCount;

        name.trim(); name.toLowerCase();
        model.trim(); model.toLowerCase();
        manufacturer.trim(); manufacturer.toLowerCase();

        uint8_t identityScore = 0U;
        if (candidateName.length() > 0U && name == candidateName) ++identityScore;
        if (candidateModel.length() > 0U && model == candidateModel) ++identityScore;
        if (candidateManufacturer.length() > 0U &&
            manufacturer == candidateManufacturer)
        {
            ++identityScore;
        }
        if (identityScore > 0U)
        {
            if (identityMatchCount == 0xFFFFU)
            {
                file.close();
                return false;
            }
            ++identityMatchCount;
        }

        if (returnedCount >= MaxListPageSize)
        {
            itemsTruncated = true;
            continue;
        }

        if (!first) json += ',';
        first = false;
        json += F("{\"identity_score\":");
        json += identityScore;
        json += F(",\"match_level\":\"");
        json += identityScore >= 2U ? F("LIKELY_SAME_MOTOR") :
                identityScore == 1U ? F("POSSIBLE_ANALOGUE") :
                                      F("SAME_PROGRAM_ONLY");
        json += F("\",\"motor\":");
        json += line;
        json += '}';
        ++returnedCount;
    }
    file.close();
    return true;
}
}
