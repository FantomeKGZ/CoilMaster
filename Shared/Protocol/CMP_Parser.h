/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Parser.h
Module    : Shared/Protocol

Description:
Packet Parser

Converts incoming byte stream from CMP_Buffer
into validated CMP_Packet objects.
==========================================================
*/

#ifndef CMP_PARSER_H
#define CMP_PARSER_H

#include <stdint.h>

#include "CMP_Buffer.h"
#include "CMP_Header.h"
#include "CMP_Packet.h"
#include "CMP_Result.h"

class CMP_Parser
{
public:

    //------------------------------------------------------
    // Parse packet
    //------------------------------------------------------

    static CMP_Result parse(
        CMP_Buffer& buffer,
        CMP_Packet& packet);

private:

    //------------------------------------------------------
    // Synchronization
    //------------------------------------------------------

    static bool findStartWord(
        CMP_Buffer& buffer);

    //------------------------------------------------------
    // Reading
    //------------------------------------------------------

    static bool readHeader(
        CMP_Buffer& buffer,
        CMP_Header& header);

    static bool readPayload(
        CMP_Buffer& buffer,
        uint8_t* payload,
        uint16_t length);

    static bool readCRC(
        CMP_Buffer& buffer,
        uint16_t& crc);

    //------------------------------------------------------
    // Validation
    //------------------------------------------------------

    static CMP_Result validateHeader(
        const CMP_Header& header);

    static CMP_Result validateCRC(
        const CMP_Packet& packet);
};

#endif