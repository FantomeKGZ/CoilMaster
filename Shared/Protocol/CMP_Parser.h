#ifndef CMP_PARSER_H
#define CMP_PARSER_H

#include <stdint.h>

#include "CMP_Buffer.h"
#include "CMP_Header.h"
#include "CMP_Packet.h"
#include "CMP_Result.h"

namespace CMP
{
class Parser
{
public:
    Parser() = delete;

    static Result parse(Buffer& buffer, Packet& packet);

private:
    static Result synchronize(Buffer& buffer);
    static Result decodeHeader(const Buffer& buffer, Header& header);
    static Result validateHeader(const Header& header);
    static uint16_t calculatePacketCRC(const Buffer& buffer,
                                       uint16_t payloadLength);

    static uint16_t readUInt16LE(const Buffer& buffer,
                                 uint16_t offset);
};
}

#endif // CMP_PARSER_H
