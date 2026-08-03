#include "CMP_Protocol.h"

#include "CMP_CRC.h"
#include "CMP_Defines.h"

namespace CMP
{
Protocol::Protocol()
    : m_rxBuffer(),
      m_nextCounter(0U)
{
}

void Protocol::reset()
{
    m_rxBuffer.clear();
    m_nextCounter = 0U;
}

Result Protocol::receive(uint8_t byte)
{
    return m_rxBuffer.push(byte);
}

Result Protocol::receive(const uint8_t* data, uint16_t length)
{
    return m_rxBuffer.push(data, length);
}

Result Protocol::read(Packet& packet)
{
    return Parser::parse(m_rxBuffer, packet);
}

Result Protocol::createPacket(Command command,
                              Flags flags,
                              const uint8_t* payload,
                              uint16_t payloadLength,
                              Packet& packet)
{
    if (payloadLength > MaxPayloadSize)
    {
        return Result::InvalidLength;
    }

    if (payloadLength > 0U && payload == nullptr)
    {
        return Result::InvalidArgument;
    }

    if (hasFlag(flags, Flags::Reserved) ||
        (hasFlag(flags, Flags::Ack) && hasFlag(flags, Flags::Nack)))
    {
        return Result::InvalidFlags;
    }

    packet.header.startWord = StartWord;
    packet.header.versionMajor = ProtocolVersionMajor;
    packet.header.versionMinor = ProtocolVersionMinor;
    packet.header.flags = flags;
    packet.header.reserved = 0U;
    packet.header.command = command;
    packet.header.counter = m_nextCounter;
    packet.header.payloadLength = payloadLength;
    packet.crc = 0U;

    for (uint16_t index = 0U; index < payloadLength; ++index)
    {
        packet.payload[index] = payload[index];
    }

    ++m_nextCounter;
    return Result::Ok;
}

Result Protocol::encode(const Packet& packet,
                        uint8_t* output,
                        uint16_t outputCapacity,
                        uint16_t& bytesWritten) const
{
    bytesWritten = 0U;

    if (output == nullptr)
    {
        return Result::InvalidArgument;
    }

    if (packet.header.startWord != StartWord)
    {
        return Result::InvalidStartWord;
    }

    if (packet.header.versionMajor != ProtocolVersionMajor ||
        packet.header.versionMinor > ProtocolVersionMinor)
    {
        return Result::InvalidVersion;
    }

    if (packet.header.payloadLength > MaxPayloadSize)
    {
        return Result::InvalidLength;
    }

    if (packet.header.reserved != 0U ||
        hasFlag(packet.header.flags, Flags::Reserved) ||
        (hasFlag(packet.header.flags, Flags::Ack) &&
         hasFlag(packet.header.flags, Flags::Nack)))
    {
        return Result::InvalidFlags;
    }

    const uint16_t packetSize = static_cast<uint16_t>(
        HeaderSize + packet.header.payloadLength + CRCSize);

    if (outputCapacity < packetSize)
    {
        return Result::BufferFull;
    }

    writeUInt16LE(output, 0U, packet.header.startWord);
    output[2U] = packet.header.versionMajor;
    output[3U] = packet.header.versionMinor;
    output[4U] = toByte(packet.header.flags);
    output[5U] = packet.header.reserved;
    writeUInt16LE(output,
                  6U,
                  static_cast<uint16_t>(packet.header.command));
    writeUInt16LE(output, 8U, packet.header.counter);
    writeUInt16LE(output, 10U, packet.header.payloadLength);

    for (uint16_t index = 0U;
         index < packet.header.payloadLength;
         ++index)
    {
        output[HeaderSize + index] = packet.payload[index];
    }

    const uint16_t crc = CRC::calculate(
        output,
        static_cast<uint16_t>(HeaderSize + packet.header.payloadLength));

    writeUInt16LE(output,
                  static_cast<uint16_t>(HeaderSize +
                                        packet.header.payloadLength),
                  crc);

    bytesWritten = packetSize;
    return Result::Ok;
}

uint16_t Protocol::bufferedBytes() const
{
    return m_rxBuffer.size();
}

uint16_t Protocol::nextCounter() const
{
    return m_nextCounter;
}

void Protocol::writeUInt16LE(uint8_t* output,
                             uint16_t offset,
                             uint16_t value)
{
    output[offset] = static_cast<uint8_t>(value & 0x00FFU);
    output[static_cast<uint16_t>(offset + 1U)] =
        static_cast<uint8_t>((value >> 8U) & 0x00FFU);
}
}
