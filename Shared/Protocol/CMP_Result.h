/*
==========================================================
CoilMaster OS

File        : CMP_Result.h
Module      : Shared/Protocol
Version     : 1.0.0
Status      : RC1

Description :
Result codes returned by CMP public operations.
The type is platform-independent, allocation-free and
safe to use on Arduino UNO and ESP32.

Copyright (c) CoilMaster Project
==========================================================
*/

#ifndef CMP_RESULT_H
#define CMP_RESULT_H

#include <stdint.h>

#include "CMP_Defines.h"

namespace CMP
{
    enum class Result : uint8_t
    {
        Ok = 0x00,

        NeedMoreData      = 0x01,
        NoPacketAvailable = 0x02,

        InvalidArgument    = 0x10,
        InvalidStartWord   = 0x11,
        UnsupportedVersion = 0x12,
        InvalidFlags       = 0x13,
        InvalidCommand     = 0x14,
        InvalidLength      = 0x15,
        PayloadTooLarge    = 0x16,
        CRCMismatch        = 0x17,

        BufferEmpty = 0x20,
        BufferFull  = 0x21,

        Timeout        = 0x30,
        TransportError = 0x31,

        HandlerNotFound = 0x40,
        CommandRejected = 0x41,

        InternalError = 0xFF
    };

    constexpr bool succeeded(const Result result)
    {
        return result == Result::Ok;
    }

    constexpr bool failed(const Result result)
    {
        return !succeeded(result);
    }

    constexpr bool isPending(const Result result)
    {
        return result == Result::NeedMoreData ||
               result == Result::NoPacketAvailable;
    }
}

static_assert(
    sizeof(CMP::Result) == sizeof(uint8_t),
    "CMP::Result must remain one byte wide.");

#endif // CMP_RESULT_H
