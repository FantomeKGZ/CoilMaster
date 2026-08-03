#ifndef CMP_PROTOCOL_H
#define CMP_PROTOCOL_H

#include <stdint.h>

#include "CMP_Buffer.h"
#include "CMP_Flags.h"
#include "CMP_Packet.h"
#include "CMP_Parser.h"
#include "CMP_Result.h"

namespace CMP
{
class Protocol
{
public:
    Protocol();

    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;

    void reset();

    Result receive(uint8_t byte);
    Result receive(const uint8_t* data, uint16_t length);

    Result read(Packet& packet);

    Result createPacket(Command command,
                        Flags flags,
                        const uint8_t* payload,
                        uint16_t payloadLength,
                        Packet& packet);

    Result encode(const Packet& packet,
                  uint8_t* output,
                  uint16_t outputCapacity,
                  uint16_t& bytesWritten) const;

    uint16_t bufferedBytes() const;
    uint16_t nextCounter() const;

private:
    static void writeUInt16LE(uint8_t* output,
                              uint16_t offset,
                              uint16_t value);

    Buffer m_rxBuffer;
    uint16_t m_nextCounter;
};
}

#endif // CMP_PROTOCOL_H
