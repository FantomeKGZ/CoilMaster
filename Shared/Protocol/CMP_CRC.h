#ifndef CMP_CRC_H
#define CMP_CRC_H

#include <stdint.h>

namespace CMP
{
class CRC
{
public:
    CRC() = delete;

    static uint16_t begin();
    static uint16_t update(uint16_t crc, uint8_t byte);
    static uint16_t update(uint16_t crc,
                           const uint8_t* data,
                           uint16_t length);
    static uint16_t calculate(const uint8_t* data,
                              uint16_t length);
};
}

#endif // CMP_CRC_H
