#include "CM_HallCalibrationDoneProtocol.h"

#include <stdlib.h>
#include <string.h>

#include "../../../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{

HallCalibrationDone::HallCalibrationDone()
    : measurementId(0UL), valid(false)
{
}

namespace HallCalibrationDoneProtocol
{
namespace
{
bool parseUnsigned32(const char* text, uint32_t& value)
{
    value = 0UL;
    if (text == nullptr || *text == '\0') return false;
    if (text[0] == '0' && text[1] != '\0') return false;

    for (const char* cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor < '0' || *cursor > '9') return false;
        const uint8_t digit = static_cast<uint8_t>(*cursor - '0');
        if (value > (0xFFFFFFFFUL - digit) / 10UL) return false;
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

bool parseDone(char* line, HallCalibrationDone& done)
{
    done = HallCalibrationDone();
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
    char* measurementText = strtok_r(nullptr, "|", &save);
    char* capability = strtok_r(nullptr, "|", &save);
    char* extra = strtok_r(nullptr, "|", &save);

    if (version == nullptr || category == nullptr || measurementText == nullptr ||
        capability == nullptr || extra != nullptr ||
        strcmp(version, "CMP1") != 0 || strcmp(category, "CAL_DONE") != 0 ||
        strcmp(capability, "C") != 0 ||
        !parseUnsigned32(measurementText, done.measurementId) ||
        done.measurementId == 0UL)
    {
        return false;
    }

    done.valid = true;
    return true;
}

} // namespace HallCalibrationDoneProtocol
} // namespace CM
