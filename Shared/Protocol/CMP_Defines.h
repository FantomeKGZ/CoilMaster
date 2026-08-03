/*
==========================================================
CoilMaster OS

File        : CMP_Defines.h
Module      : Shared/Protocol
Version     : 1.0.0
Status      : RC1

Description :
Global protocol constants and configuration.
This file is the foundation of the CMP protocol and
contains only compile-time constants.

Copyright (c) CoilMaster Project
==========================================================
*/

#ifndef CMP_DEFINES_H
#define CMP_DEFINES_H

#include <stdint.h>

namespace CMP
{
    //======================================================
    // Protocol Version
    //======================================================

    constexpr uint8_t ProtocolVersionMajor = 1;
    constexpr uint8_t ProtocolVersionMinor = 0;

    //======================================================
    // Packet Format
    //======================================================

    constexpr uint16_t StartWord = 0xAA55;

    //======================================================
    // Payload
    //======================================================

    constexpr uint16_t MaxPayloadSize = 128;

    //======================================================
    // Buffers
    //======================================================

    constexpr uint16_t RxBufferSize = 256;
    constexpr uint16_t TxBufferSize = 256;

    //======================================================
    // CRC16-CCITT
    //======================================================

    constexpr uint16_t CRCInitialValue = 0xFFFF;
    constexpr uint16_t CRCPolynomial   = 0x1021;

    //======================================================
    // UART
    //======================================================

    constexpr uint32_t DefaultBaudRate = 115200UL;

    //======================================================
    // Packet Limits
    //======================================================

    constexpr uint16_t HeaderSize = 12;
    constexpr uint16_t CRCSize    = 2;

    constexpr uint16_t MaxPacketSize =
        HeaderSize +
        MaxPayloadSize +
        CRCSize;
}

//==========================================================
// Compile-Time Validation
//==========================================================

static_assert(
    CMP::HeaderSize == 12,
    "Invalid CMP header size.");

static_assert(
    CMP::MaxPayloadSize > 0,
    "Payload size must be greater than zero.");

static_assert(
    CMP::RxBufferSize >= CMP::MaxPacketSize,
    "RX buffer is too small for one complete packet.");

static_assert(
    CMP::TxBufferSize >= CMP::MaxPacketSize,
    "TX buffer is too small for one complete packet.");

#endif // CMP_DEFINES_H
