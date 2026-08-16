#ifndef CM_CMP1_CRC_H
#define CM_CMP1_CRC_H

#include <stddef.h>
#include <stdint.h>

namespace CM
{
namespace Cmp1Crc
{
static const uint16_t InitialValue = 0xFFFFU;
static const uint16_t Polynomial = 0xA001U;

inline uint16_t update(uint16_t crc, const uint8_t* data, size_t length)
{
    if (data == nullptr) return crc;

    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 1U) != 0U
                      ? static_cast<uint16_t>((crc >> 1U) ^ Polynomial)
                      : static_cast<uint16_t>(crc >> 1U);
        }
    }
    return crc;
}

inline uint16_t calculate(const uint8_t* data, size_t length)
{
    return update(InitialValue, data, length);
}

inline uint16_t calculate(const char* data, size_t length)
{
    return calculate(reinterpret_cast<const uint8_t*>(data), length);
}
}
}

#endif // CM_CMP1_CRC_H
