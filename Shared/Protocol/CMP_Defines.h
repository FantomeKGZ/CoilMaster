/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Defines.h
Module    : Shared/Protocol

Release   : 0.1.0
Build     : 002A
Package   : 01.3

Description:
Global protocol constants.
==========================================================
*/

#ifndef CMP_DEFINES_H
#define CMP_DEFINES_H

#include <stdint.h>

//==========================================================
// Protocol Version
//==========================================================

#define CMP_PROTOCOL_VERSION_MAJOR     1
#define CMP_PROTOCOL_VERSION_MINOR     0

//==========================================================
// Packet Signature
//==========================================================

#define CMP_START_WORD                 0xAA55

//==========================================================
// Payload
//==========================================================

#define CMP_MAX_PAYLOAD_SIZE           128

//==========================================================
// Communication
//==========================================================

#define CMP_DEFAULT_BAUDRATE           115200

//==========================================================
// Timeouts
//==========================================================

#define CMP_RX_TIMEOUT_MS              100
#define CMP_TX_TIMEOUT_MS              100

//==========================================================
// Buffer Sizes
//==========================================================

#define CMP_RX_BUFFER_SIZE             256
#define CMP_TX_BUFFER_SIZE             256

//==========================================================
// Protocol Limits
//==========================================================

#define CMP_MAX_PACKET_SIZE \
(
    sizeof(uint16_t) +     /* Start Word      */
    sizeof(uint8_t)  +     /* Version Major   */
    sizeof(uint8_t)  +     /* Version Minor   */
    sizeof(uint16_t) +     /* Command         */
    sizeof(uint16_t) +     /* Counter         */
    sizeof(uint16_t) +     /* Payload Length  */
    CMP_MAX_PAYLOAD_SIZE +
    sizeof(uint16_t)       /* CRC16           */
)

#endif