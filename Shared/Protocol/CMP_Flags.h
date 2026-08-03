#ifndef CMP_FLAGS_H
#define CMP_FLAGS_H

#include <stdint.h>

namespace CMP
{
enum class Flags : uint8_t
{
    None        = 0x00U,
    AckRequired = 0x01U,
    Ack         = 0x02U,
    Nack        = 0x04U,
    Response    = 0x08U,
    Broadcast   = 0x10U,
    Compressed  = 0x20U,
    Encrypted   = 0x40U,
    Reserved    = 0x80U
};

constexpr Flags operator|(Flags lhs, Flags rhs)
{
    return static_cast<Flags>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr Flags operator&(Flags lhs, Flags rhs)
{
    return static_cast<Flags>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline Flags& operator|=(Flags& lhs, Flags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline Flags& operator&=(Flags& lhs, Flags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

constexpr bool hasFlag(Flags value, Flags flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0U;
}

constexpr uint8_t toByte(Flags value)
{
    return static_cast<uint8_t>(value);
}
}

#endif // CMP_FLAGS_H
