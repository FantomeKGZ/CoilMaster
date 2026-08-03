#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CMP_Buffer.h"
#include "CMP_CRC.h"
#include "CMP_Dispatcher.h"
#include "CMP_Protocol.h"

namespace
{
int g_failures = 0;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        ++g_failures;
        printf("FAIL: %s\n", message);
    }
}

void testCRC()
{
    static const uint8_t data[] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    check(CMP::CRC::calculate(data, sizeof(data)) == 0x29B1U,
          "CRC16-CCITT known vector");
    check(CMP::CRC::calculate(nullptr, 0U) == CMP::CRC::begin(),
          "CRC empty input");
}

void testBuffer()
{
    CMP::Buffer buffer;

    check(buffer.empty(), "new buffer is empty");
    check(buffer.capacity() == CMP::RxBufferSize,
          "buffer capacity matches configuration");

    for (uint16_t value = 0U; value < buffer.capacity(); ++value)
    {
        check(buffer.push(static_cast<uint8_t>(value)) == CMP::Result::Ok,
              "fill buffer");
    }

    check(buffer.full(), "filled buffer is full");
    check(buffer.push(0xAAU) == CMP::Result::BufferFull,
          "push to full buffer fails");

    for (uint16_t value = 0U; value < 100U; ++value)
    {
        uint8_t byte = 0U;
        check(buffer.pop(byte) == CMP::Result::Ok,
              "pop initial bytes");
        check(byte == static_cast<uint8_t>(value),
              "buffer preserves byte order");
    }

    for (uint16_t value = 0U; value < 100U; ++value)
    {
        check(buffer.push(static_cast<uint8_t>(value + 7U)) == CMP::Result::Ok,
              "push after wrap");
    }

    check(buffer.full(), "wrapped buffer is full");
    check(buffer.discard(buffer.size()) == CMP::Result::Ok,
          "discard all bytes");
    check(buffer.empty(), "buffer empty after discard");
}

void testProtocolRoundTrip()
{
    CMP::Protocol sender;
    CMP::Protocol receiver;
    CMP::Packet outgoing{};
    CMP::Packet incoming{};

    const uint8_t payload[] = {0x10U, 0x20U, 0x30U, 0x40U};

    check(sender.createPacket(CMP::Command::Ping,
                              CMP::Flags::AckRequired,
                              payload,
                              sizeof(payload),
                              outgoing) == CMP::Result::Ok,
          "create packet");

    uint8_t encoded[CMP::MaxPacketSize] = {};
    uint16_t encodedLength = 0U;

    check(sender.encode(outgoing,
                        encoded,
                        sizeof(encoded),
                        encodedLength) == CMP::Result::Ok,
          "encode packet");
    check(encodedLength == CMP::HeaderSize + sizeof(payload) + CMP::CRCSize,
          "encoded packet length");

    const uint8_t noise[] = {0x00U, 0x7EU, 0x55U};
    check(receiver.receive(noise, sizeof(noise)) == CMP::Result::Ok,
          "receive leading noise");

    const uint16_t split = static_cast<uint16_t>(encodedLength / 2U);
    check(receiver.receive(encoded, split) == CMP::Result::Ok,
          "receive first fragment");
    check(receiver.read(incoming) == CMP::Result::PacketIncomplete,
          "fragment is incomplete");
    check(receiver.receive(encoded + split,
                           static_cast<uint16_t>(encodedLength - split)) ==
              CMP::Result::Ok,
          "receive second fragment");
    check(receiver.read(incoming) == CMP::Result::Ok,
          "parse complete packet");

    check(incoming.header.command == CMP::Command::Ping,
          "round-trip command");
    check(incoming.header.flags == CMP::Flags::AckRequired,
          "round-trip flags");
    check(incoming.header.payloadLength == sizeof(payload),
          "round-trip payload length");
    check(memcmp(incoming.payload, payload, sizeof(payload)) == 0,
          "round-trip payload");
    check(receiver.bufferedBytes() == 0U,
          "packet removed from receiver buffer");
}

void testCorruptedCRCRecovery()
{
    CMP::Protocol sender;
    CMP::Protocol receiver;
    CMP::Packet packet{};
    CMP::Packet parsed{};

    check(sender.createPacket(CMP::Command::Ping,
                              CMP::Flags::None,
                              nullptr,
                              0U,
                              packet) == CMP::Result::Ok,
          "create zero-payload packet");

    uint8_t encoded[CMP::MaxPacketSize] = {};
    uint16_t length = 0U;
    check(sender.encode(packet, encoded, sizeof(encoded), length) ==
              CMP::Result::Ok,
          "encode zero-payload packet");

    encoded[length - 1U] ^= 0x01U;
    check(receiver.receive(encoded, length) == CMP::Result::Ok,
          "receive corrupted packet");
    check(receiver.read(parsed) == CMP::Result::InvalidCRC,
          "detect invalid CRC");
}

CMP::Result pingHandler(const CMP::Packet& packet, void* context)
{
    uint16_t* calls = static_cast<uint16_t*>(context);
    ++(*calls);
    return packet.header.command == CMP::Command::Ping
               ? CMP::Result::Ok
               : CMP::Result::Error;
}

void testDispatcher()
{
    CMP::Dispatcher dispatcher;
    CMP::Packet packet{};
    packet.header.command = CMP::Command::Ping;

    uint16_t calls = 0U;
    check(dispatcher.registerHandler(CMP::Command::Ping,
                                     pingHandler,
                                     &calls) == CMP::Result::Ok,
          "register handler");
    check(dispatcher.dispatch(packet) == CMP::Result::Ok,
          "dispatch registered command");
    check(calls == 1U, "handler called once");
    check(dispatcher.unregisterHandler(CMP::Command::Ping) ==
              CMP::Result::Ok,
          "unregister handler");
    check(dispatcher.dispatch(packet) == CMP::Result::UnknownCommand,
          "unregistered command rejected");
}
}

int main()
{
    testCRC();
    testBuffer();
    testProtocolRoundTrip();
    testCorruptedCRCRecovery();
    testDispatcher();

    if (g_failures == 0)
    {
        printf("All CMP protocol tests passed.\n");
        return 0;
    }

    printf("CMP protocol tests failed: %d\n", g_failures);
    return 1;
}
