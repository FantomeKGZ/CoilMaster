#include "CMP_Parser.h"

#include "CMP_CRC.h"
#include "CMP_Defines.h"
#include "CMP_Flags.h"

namespace CMP
{
namespace
{
constexpr uint8_t StartWordLow =
    static_cast<uint8_t>(StartWord & 0x00FFU);
constexpr uint8_t StartWordHigh =
    static_cast<uint8_t>((StartWord >> 8U) & 0x00FFU);
}

Result Parser::parse(Buffer& buffer, Packet& packet)
{
    Result result = synchronize(buffer);
    if (result != Result::Ok)
    {
        return result;
    }

    if (!buffer.contains(HeaderSize))
    {
        return Result::PacketIncomplete;
    }

    Header header{};
    result = decodeHeader(buffer, header);
    if (result != Result::Ok)
    {
        return result;
    }

    result = validateHeader(header);
    if (result != Result::Ok)
    {
        buffer.discard(1U);
        return result;
    }

    const uint16_t packetSize = static_cast<uint16_t>(
        HeaderSize + header.payloadLength + CRCSize);

    if (!buffer.contains(packetSize))
    {
        return Result::PacketIncomplete;
    }

    const uint16_t receivedCRC = readUInt16LE(
        buffer,
        static_cast<uint16_t>(HeaderSize + header.payloadLength));

    const uint16_t calculatedCRC =
        calculatePacketCRC(buffer, header.payloadLength);

    if (receivedCRC != calculatedCRC)
    {
        buffer.discard(1U);
        return Result::InvalidCRC;
    }

    packet.header = header;
    packet.crc = receivedCRC;

    if (header.payloadLength > 0U)
    {
        result = buffer.peek(HeaderSize,
                             packet.payload,
                             header.payloadLength);
        if (result != Result::Ok)
        {
            return result;
        }
    }

    return buffer.discard(packetSize);
}

Result Parser::synchronize(Buffer& buffer)
{
    while (buffer.size() >= 2U)
    {
        uint8_t low = 0U;
        uint8_t high = 0U;

        if (buffer.peek(0U, low) != Result::Ok ||
            buffer.peek(1U, high) != Result::Ok)
        {
            return Result::PacketIncomplete;
        }

        if (low == StartWordLow && high == StartWordHigh)
        {
            return Result::Ok;
        }

        buffer.discard(1U);
    }

    if (buffer.size() == 1U)
    {
        uint8_t value = 0U;
        if (buffer.peek(0U, value) == Result::Ok &&
            value != StartWordLow)
        {
            buffer.discard(1U);
        }
    }

    return Result::PacketIncomplete;
}

Result Parser::decodeHeader(const Buffer& buffer, Header& header)
{
    if (!buffer.contains(HeaderSize))
    {
        return Result::PacketIncomplete;
    }

    uint8_t value = 0U;

    header.startWord = readUInt16LE(buffer, 0U);

    if (buffer.peek(2U, header.versionMajor) != Result::Ok ||
        buffer.peek(3U, header.versionMinor) != Result::Ok ||
        buffer.peek(4U, value) != Result::Ok)
    {
        return Result::PacketIncomplete;
    }
    header.flags = static_cast<Flags>(value);

    if (buffer.peek(5U, header.reserved) != Result::Ok)
    {
        return Result::PacketIncomplete;
    }

    header.command = static_cast<Command>(readUInt16LE(buffer, 6U));
    header.counter = readUInt16LE(buffer, 8U);
    header.payloadLength = readUInt16LE(buffer, 10U);

    return Result::Ok;
}

Result Parser::validateHeader(const Header& header)
{
    if (header.startWord != StartWord)
    {
        return Result::InvalidStartWord;
    }

    if (header.versionMajor != ProtocolVersionMajor ||
        header.versionMinor > ProtocolVersionMinor)
    {
        return Result::InvalidVersion;
    }

    if (header.payloadLength > MaxPayloadSize)
    {
        return Result::InvalidLength;
    }

    if (header.reserved != 0U ||
        hasFlag(header.flags, Flags::Reserved) ||
        (hasFlag(header.flags, Flags::Ack) &&
         hasFlag(header.flags, Flags::Nack)))
    {
        return Result::InvalidFlags;
    }

    return Result::Ok;
}

uint16_t Parser::calculatePacketCRC(const Buffer& buffer,
                                    uint16_t payloadLength)
{
    uint16_t crc = CRC::begin();
    const uint16_t length =
        static_cast<uint16_t>(HeaderSize + payloadLength);

    for (uint16_t offset = 0U; offset < length; ++offset)
    {
        uint8_t value = 0U;
        if (buffer.peek(offset, value) != Result::Ok)
        {
            return 0U;
        }

        crc = CRC::update(crc, value);
    }

    return crc;
}

uint16_t Parser::readUInt16LE(const Buffer& buffer,
                              uint16_t offset)
{
    uint8_t low = 0U;
    uint8_t high = 0U;

    if (buffer.peek(offset, low) != Result::Ok ||
        buffer.peek(static_cast<uint16_t>(offset + 1U), high) !=
            Result::Ok)
    {
        return 0U;
    }

    return static_cast<uint16_t>(low) |
           static_cast<uint16_t>(static_cast<uint16_t>(high) << 8U);
}
}
