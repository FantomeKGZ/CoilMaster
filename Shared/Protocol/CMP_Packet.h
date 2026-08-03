/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Packet.h
Module    : Shared/Protocol

Description:
Packet Definition
==========================================================
*/

#ifndef CMP_PACKET_H
#define CMP_PACKET_H

#include <stdint.h>

#include "CMP_Defines.h"
#include "CMP_Header.h"

#pragma pack(push,1)

struct CMP_Packet
{
    //------------------------------------------------------
    // Header
    //------------------------------------------------------

    CMP_Header header;

    //------------------------------------------------------
    // Payload
    //------------------------------------------------------

    uint8_t payload[CMP_MAX_PAYLOAD_SIZE];

    //------------------------------------------------------
    // CRC16
    //------------------------------------------------------

    uint16_t crc;
};

#pragma pack(pop)

static_assert(
    sizeof(CMP_Packet) ==
    sizeof(CMP_Header)
    + CMP_MAX_PAYLOAD_SIZE
    + sizeof(uint16_t),
    "CMP_Packet size mismatch");

#endif