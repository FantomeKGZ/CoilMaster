/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Result.h
Module    : Shared/Protocol

Description:
CMP operation result codes.

Release   : 0.1.0
Build     : 002A
Package   : 01.3
==========================================================
*/

#ifndef CMP_RESULT_H
#define CMP_RESULT_H

#include <stdint.h>

enum class CMP_Result : uint8_t
{
    //------------------------------------------------------
    // Success
    //------------------------------------------------------

    OK = 0,

    //------------------------------------------------------
    // General
    //------------------------------------------------------

    ERROR,

    TIMEOUT,

    //------------------------------------------------------
    // Transport
    //------------------------------------------------------

    SERIAL_NOT_INITIALIZED,

    RX_BUFFER_EMPTY,

    RX_BUFFER_OVERFLOW,

    TX_BUFFER_OVERFLOW,

    //------------------------------------------------------
    // Packet
    //------------------------------------------------------

    INVALID_START_WORD,

    INVALID_VERSION,

    INVALID_FLAGS,

    INVALID_LENGTH,

    INVALID_CRC,

    PACKET_TOO_LARGE,

    PACKET_INCOMPLETE,

    //------------------------------------------------------
    // Protocol
    //------------------------------------------------------

    UNKNOWN_COMMAND,

    NOT_SUPPORTED,

    BUSY,

    //------------------------------------------------------
    // Application
    //------------------------------------------------------

    INVALID_PARAMETER,

    INTERNAL_ERROR
};

#endif