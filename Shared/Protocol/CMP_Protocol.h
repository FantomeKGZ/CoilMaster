/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Protocol.h
Module    : Shared/Protocol

Description:
Protocol Engine
==========================================================
*/

#ifndef CMP_PROTOCOL_H
#define CMP_PROTOCOL_H

#include <Arduino.h>

#include "CMP_Buffer.h"
#include "CMP_Command.h"
#include "CMP_CRC.h"
#include "CMP_Packet.h"

class CMP_Protocol
{
public:

    //------------------------------------------------------
    // Initialization
    //------------------------------------------------------

    static void begin(HardwareSerial& serial);

    //------------------------------------------------------
    // Poll serial port
    //------------------------------------------------------

    static void update();

    //------------------------------------------------------
    // Send
    //------------------------------------------------------

    static bool send(
        CMP_Command command,
        const uint8_t* payload = nullptr,
        uint16_t payloadLength = 0);

    //------------------------------------------------------
    // Receive
    //------------------------------------------------------

    static bool available();

    static bool read(CMP_Packet& packet);

private:

    //------------------------------------------------------
    // UART
    //------------------------------------------------------

    static HardwareSerial* _serial;

    //------------------------------------------------------
    // RX Buffer
    //------------------------------------------------------

    static CMP_Buffer _rxBuffer;

    //------------------------------------------------------
    // Packet Counter
    //------------------------------------------------------

    static uint16_t _packetCounter;

    //------------------------------------------------------
    // Last Packet
    //------------------------------------------------------

    static CMP_Packet _packet;

    static bool _packetReady;

    //------------------------------------------------------
    // Parser
    //------------------------------------------------------

    static bool parsePacket();
};

#endif