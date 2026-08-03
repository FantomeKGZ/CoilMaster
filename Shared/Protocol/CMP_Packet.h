#ifndef CMP_PACKET_H
#define CMP_PACKET_H

#include <stdint.h>

#include "CMP_Defines.h"
#include "CMP_Header.h"

namespace CMP
{
struct Packet
{
    Header header;
    uint8_t payload[MaxPayloadSize];
    uint16_t crc;
};
}

#endif // CMP_PACKET_H
