#include "CM_HallCalibrationRawProtocol.h"

#include <stdlib.h>
#include <string.h>

#include "../../../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{

HallCalibrationRawSample::HallCalibrationRawSample()
    : phase(HallCalibrationRawPhase::Baseline), rawAdc(0U), sequence(0U),
      elapsedMs(0UL), valid(false)
{
}

namespace HallCalibrationRawProtocol
{
namespace
{
bool parseUnsigned(const char* text, uint32_t maximum, uint32_t& value)
{
    value = 0UL;
    if (text == nullptr || *text == '\0') return false;
    if (text[0] == '0' && text[1] != '\0') return false;

    for (const char* cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor < '0' || *cursor > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(*cursor - '0');
        if (value > (maximum - digit) / 10UL) return false;
        value = value * 10UL + digit;
    }
    return true;
}

bool parseHex16(const char* text, uint16_t& value)
{
    if (text == nullptr || strlen(text) != 4U) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text, &end, 16);
    if (end == nullptr || *end != '\0' || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}
}

bool parseSample(char* line, HallCalibrationRawSample& sample)
{
    sample = HallCalibrationRawSample();
    if (line == nullptr) return false;

    char* lastSeparator = strrchr(line, '|');
    if (lastSeparator == nullptr) return false;

    uint16_t receivedCrc = 0U;
    if (!parseHex16(lastSeparator + 1, receivedCrc)) return false;
    const size_t payloadLength = static_cast<size_t>(lastSeparator - line);
    if (Cmp1Crc::calculate(line, payloadLength) != receivedCrc) return false;
    *lastSeparator = '\0';

    char* save = nullptr;
    char* version = strtok_r(line, "|", &save);
    char* category = strtok_r(nullptr, "|", &save);
    char* phaseText = strtok_r(nullptr, "|", &save);
    char* rawText = strtok_r(nullptr, "|", &save);
    char* sequenceText = strtok_r(nullptr, "|", &save);
    char* elapsedText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || phaseText == nullptr ||
        rawText == nullptr || sequenceText == nullptr || elapsedText == nullptr ||
        capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "CAL_SAMPLE") != 0 ||
        strcmp(capability, "C") != 0)
    {
        return false;
    }

    if (strcmp(phaseText, "BASELINE") == 0)
        sample.phase = HallCalibrationRawPhase::Baseline;
    else if (strcmp(phaseText, "RUN") == 0)
        sample.phase = HallCalibrationRawPhase::Run;
    else
        return false;

    uint32_t raw = 0UL;
    uint32_t sequence = 0UL;
    uint32_t elapsed = 0UL;
    if (!parseUnsigned(rawText, 1023UL, raw) ||
        !parseUnsigned(sequenceText, 65535UL, sequence) ||
        !parseUnsigned(elapsedText, 0xFFFFFFFFUL, elapsed))
    {
        return false;
    }

    sample.rawAdc = static_cast<uint16_t>(raw);
    sample.sequence = static_cast<uint16_t>(sequence);
    sample.elapsedMs = elapsed;
    sample.valid = true;
    return true;
}

} // namespace HallCalibrationRawProtocol
} // namespace CM
