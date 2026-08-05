#include "CM_RepairRegistry.h"

namespace CM
{
bool RepairRegistry::appendSimilarMotorsJson(String& json,
                                             const NewMotor& candidate,
                                             uint16_t& sameProgramCount,
                                             uint16_t& identityMatchCount) const
{
    sameProgramCount = 0U;
    identityMatchCount = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(MotorsPath)) return true;

    String candidateName = candidate.name;
    String candidateModel = candidate.model;
    String candidateManufacturer = candidate.manufacturer;
    String candidateProgram = candidate.coilProgram;
    candidateName.trim(); candidateName.toLowerCase();
    candidateModel.trim(); candidateModel.toLowerCase();
    candidateManufacturer.trim(); candidateManufacturer.toLowerCase();
    candidateProgram.trim();

    File file = m_storage.open(MotorsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        String program;
        if (!findString(line, "coil_program", program)) continue;
        program.trim();
        if (program != candidateProgram) continue;

        ++sameProgramCount;
        String name, model, manufacturer;
        findString(line, "name", name);
        findString(line, "model", model);
        findString(line, "manufacturer", manufacturer);
        name.trim(); name.toLowerCase();
        model.trim(); model.toLowerCase();
        manufacturer.trim(); manufacturer.toLowerCase();

        uint8_t identityScore = 0U;
        if (candidateName.length() > 0U && name == candidateName) ++identityScore;
        if (candidateModel.length() > 0U && model == candidateModel) ++identityScore;
        if (candidateManufacturer.length() > 0U && manufacturer == candidateManufacturer) ++identityScore;
        if (identityScore > 0U) ++identityMatchCount;

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
    }
    file.close();
    return true;
}
}
