#pragma once

#include <stddef.h>
#include <stdint.h>

namespace CM
{
namespace CrcFrameText
{
bool formatSuffix(uint16_t crc, char* output, size_t outputSize);
bool append(char* output, size_t outputSize, size_t payloadLength);
}
}
