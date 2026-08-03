/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Parser.cpp
Module    : Shared/Protocol

Description:
CMP Packet Parser
==========================================================
*/

#include "CMP_Parser.h"

#include <cstring>

#include "CMP_CRC.h"
#include "CMP_Defines.h"

//----------------------------------------------------------
// Public
//----------------------------------------------------------

CMP_Result CMP_Parser::parse(
    CMP_Buffer& buffer,
    CMP_Packet& packet)
{
    //------------------------------------------------------
    // Search packet start
    //------------------------------------------------------

    if (!findStartWord(buffer))
    {
        return CMP_Result::PACKET_INCOMPLETE;
    }

    //------------------------------------------------------
    // Header
    //------------------------------------------------------

    if (!readHeader(buffer, packet.header))
    {
        return CMP_Result::PACKET_INCOMPLETE;
    }

    //------------------------------------------------------
    // Validate header
    //------------------------------------------------------

    CMP_Result result = validateHeader(packet.header);

    if (result != CMP_Result::OK)
    {
        return result;
    }

    //------------------------------------------------------
    // Payload
    //------------------------------------------------------

    if (!readPayload(
            buffer,
            packet.payload,
            packet.header.payloadLength))
    {
        return CMP_Result::PACKET_INCOMPLETE;
    }

    //------------------------------------------------------
    // CRC
    //------------------------------------------------------

    if (!readCRC(buffer, packet.crc))
    {
        return CMP_Result::PACKET_INCOMPLETE;
    }

    //------------------------------------------------------
    // CRC Validation
    //------------------------------------------------------

    result = validateCRC(packet);

    if (result != CMP_Result::OK)
    {
        return result;
    }

    return CMP_Result::OK;
}

//----------------------------------------------------------
// Private
//----------------------------------------------------------

bool CMP_Parser::findStartWord(
    CMP_Buffer& buffer)
{
    while (buffer.available() >= 2)
    {
        uint8_t b0;
        uint8_t b1;

        buffer.peek(0, b0);
        buffer.peek(1, b1);

        uint16_t word =
            ((uint16_t)b0 << 8) |
             (uint16_t)b1;

        if (word == CMP_START_WORD)
        {
            return true;
        }

        buffer.discard(1);
    }

    return false;
}

//----------------------------------------------------------
// Read Header
//----------------------------------------------------------

bool CMP_Parser::readHeader(
    CMP_Buffer& buffer,
    CMP_Header& header)
{
    if (buffer.available() < sizeof(CMP_Header))
    {
        return false;
    }

    uint8_t rawHeader[sizeof(CMP_Header)];

    if (!buffer.read(rawHeader, sizeof(rawHeader)))
    {
        return false;
    }

    memcpy(&header, rawHeader, sizeof(CMP_Header));

    return true;
}

//----------------------------------------------------------
// Validate Header
//----------------------------------------------------------

CMP_Result CMP_Parser::validateHeader(
    const CMP_Header& header)
{
    //------------------------------------------------------
    // Start Word
    //------------------------------------------------------

    if (header.startWord != CMP_START_WORD)
    {
        return CMP_Result::INVALID_START_WORD;
    }

    //------------------------------------------------------
    // Protocol Version
    //------------------------------------------------------

    if (header.versionMajor !=
        CMP_PROTOCOL_VERSION_MAJOR)
    {
        return CMP_Result::INVALID_VERSION;
    }

    if (header.versionMinor >
        CMP_PROTOCOL_VERSION_MINOR)
    {
        return CMP_Result::INVALID_VERSION;
    }

    //------------------------------------------------------
    // Payload Length
    //------------------------------------------------------

    if (header.payloadLength >
        CMP_MAX_PAYLOAD_SIZE)
    {
        return CMP_Result::INVALID_LENGTH;
    }

    return CMP_Result::OK;
}

//----------------------------------------------------------
// Read Payload
//----------------------------------------------------------

bool CMP_Parser::readPayload(
    CMP_Buffer& buffer,
    uint8_t* payload,
    uint16_t length)
{
    if (length == 0)
    {
        return true;
    }

    if (buffer.available() < length)
    {
        return false;
    }

    return buffer.read(payload, length);
}
//----------------------------------------------------------
// Read CRC
//----------------------------------------------------------

bool CMP_Parser::readCRC(
    CMP_Buffer& buffer,
    uint16_t& crc)
{
    if (buffer.available() < sizeof(uint16_t))
    {
        return false;
    }

    uint8_t rawCRC[2];

    if (!buffer.read(rawCRC, sizeof(rawCRC)))
    {
        return false;
    }

    crc =
        (uint16_t)rawCRC[0] |
        ((uint16_t)rawCRC[1] << 8);

    return true;
}

//----------------------------------------------------------
// Validate CRC
//----------------------------------------------------------

CMP_Result CMP_Parser::validateCRC(
    const CMP_Packet& packet)
{
    uint16_t crc = CMP_CRC::begin();

    //------------------------------------------------------
    // Header
    //------------------------------------------------------

    crc = CMP_CRC::update(
        crc,
        reinterpret_cast<const uint8_t*>(&packet.header),
        sizeof(CMP_Header));

    //------------------------------------------------------
    // Payload
    //------------------------------------------------------

    if (packet.header.payloadLength > 0)
    {
        crc = CMP_CRC::update(
            crc,
            packet.payload,
            packet.header.payloadLength);
    }

    //------------------------------------------------------
    // Compare
    //------------------------------------------------------

    if (crc != packet.crc)
    {
        return CMP_Result::INVALID_CRC;
    }

    return CMP_Result::OK;
}

//----------------------------------------------------------
// End of File
//----------------------------------------------------------