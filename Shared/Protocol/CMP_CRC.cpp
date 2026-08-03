#include "CMP_CRC.h"

#include "CMP_Defines.h"

namespace CMP
{
uint16_t CRC::begin()
{
    return CRCInitialValue;
}

uint16_t CRC::update(uint16_t crc, uint8_t byte)
{
    crc ^= static_cast<uint16_t>(byte) << 8U;

    for (uint8_t bit = 0U; bit < 8U; ++bit)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = static_cast<uint16_t>((crc << 1U) ^ CRCPolynomial);
        }
        else
        {
            crc = static_cast<uint16_t>(crc << 1U);
        }
    }

    return crc;
}

uint16_t CRC::update(uint16_t crc,
                     const uint8_t* data,
                     uint16_t length)
{
    if (data == nullptr)
    {
        return crc;
    }

    for (uint16_t index = 0U; index < length; ++index)
    {
        crc = update(crc, data[index]);
    }

    return crc;
}

uint16_t CRC::calculate(const uint8_t* data,
                        uint16_t length)
{
    return update(begin(), data, length);
}
}
