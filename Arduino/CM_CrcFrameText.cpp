#include "CM_CrcFrameText.h"

#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{
namespace CrcFrameText
{
namespace
{
char hexDigit(uint8_t value)
{
    value &= 0x0FU;
    return static_cast<char>(value < 10U ? ('0' + value) : ('A' + value - 10U));
}
}

bool formatSuffix(uint16_t crc, char* output, size_t outputSize)
{
    if (output == nullptr || outputSize < 7U) return false;
    output[0] = '|';
    output[1] = hexDigit(static_cast<uint8_t>(crc >> 12U));
    output[2] = hexDigit(static_cast<uint8_t>(crc >> 8U));
    output[3] = hexDigit(static_cast<uint8_t>(crc >> 4U));
    output[4] = hexDigit(static_cast<uint8_t>(crc));
    output[5] = '\n';
    output[6] = '\0';
    return true;
}

bool append(char* output, size_t outputSize, size_t payloadLength)
{
    if (output == nullptr || payloadLength == 0U || payloadLength >= outputSize)
        return false;
    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output), payloadLength);
    return formatSuffix(crc, output + payloadLength, outputSize - payloadLength);
}
}
}
