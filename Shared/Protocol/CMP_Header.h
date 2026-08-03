/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Header.h
Module    : Shared/Protocol

Description:
CMP Packet Header

This file defines the binary header format used
by all CMP packets.

WARNING:
Changing this structure changes the protocol.
==========================================================
*/

#ifndef CMP_HEADER_H
#define CMP_HEADER_H

#include <stdint.h>

#include "CMP_Defines.h"

#pragma pack(push,1)

struct CMP_Header
{
    //------------------------------------------------------
    // Packet Signature
    //------------------------------------------------------

    uint16_t startWord;

    //------------------------------------------------------
    // Protocol Version
    //------------------------------------------------------

    uint8_t versionMajor;

    uint8_t versionMinor;

    //------------------------------------------------------
    // Packet Flags
    //------------------------------------------------------

    uint8_t flags;

    //------------------------------------------------------
    // Reserved
    //------------------------------------------------------

    uint8_t reserved;

    //------------------------------------------------------
    // Command
    //------------------------------------------------------

    uint16_t command;

    //------------------------------------------------------
    // Packet Counter
    //------------------------------------------------------

    uint16_t counter;

    //------------------------------------------------------
    // Payload Length
    //------------------------------------------------------

    uint16_t payloadLength;
};

#pragma pack(pop)

static_assert(
    sizeof(CMP_Header) == 12,
    "CMP_Header must be exactly 12 bytes");

#endif