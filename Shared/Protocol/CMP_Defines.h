#ifndef CMP_DEFINES_H
#define CMP_DEFINES_H

#include <stdint.h>

namespace CMP
{
constexpr uint8_t ProtocolVersionMajor = 1U;
constexpr uint8_t ProtocolVersionMinor = 0U;
constexpr uint16_t StartWord = 0xAA55U;
constexpr uint16_t HeaderSize = 12U;
constexpr uint16_t CRCSize = 2U;
constexpr uint16_t MaxPayloadSize = 128U;
constexpr uint16_t MaxPacketSize = HeaderSize + MaxPayloadSize + CRCSize;
constexpr uint16_t RxBufferSize = 256U;
constexpr uint16_t TxBufferSize = 256U;
constexpr uint32_t DefaultBaudRate = 115200UL;
constexpr uint16_t CRCInitialValue = 0xFFFFU;
constexpr uint16_t CRCPolynomial = 0x1021U;
}

static_assert(CMP::RxBufferSize >= CMP::MaxPacketSize,
              "CMP RX buffer must hold one maximum packet.");
static_assert(CMP::TxBufferSize >= CMP::MaxPacketSize,
              "CMP TX buffer must hold one maximum packet.");

#endif // CMP_DEFINES_H
