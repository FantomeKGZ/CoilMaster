#ifndef CMP_COMMAND_H
#define CMP_COMMAND_H

#include <stdint.h>

namespace CMP
{
enum class Command : uint16_t
{
    None   = 0x0000U,

    Ping   = 0x0001U,
    Pong   = 0x0002U,
    Ack    = 0x0003U,
    Nack   = 0x0004U,

    Read   = 0x0010U,
    Write  = 0x0011U,
    Notify = 0x0012U,

    Reset  = 0x0020U,
    Error  = 0x0021U,

    Reserved = 0xFFFEU,
    Unknown  = 0xFFFFU
};
}

#endif // CMP_COMMAND_H
