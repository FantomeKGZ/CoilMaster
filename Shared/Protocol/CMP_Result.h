#ifndef CMP_RESULT_H
#define CMP_RESULT_H

#include <stdint.h>

namespace CMP
{
enum class Result : uint8_t
{
    Ok = 0U,

    Error,
    InvalidArgument,
    NotInitialized,
    NotSupported,
    Busy,
    Timeout,

    BufferEmpty,
    BufferFull,
    BufferOverflow,

    PacketIncomplete,
    InvalidPacket,
    InvalidStartWord,
    InvalidVersion,
    InvalidFlags,
    InvalidLength,
    InvalidCRC,

    UnknownCommand,
    TxFailed,
    RxFailed
};
}

#endif // CMP_RESULT_H
