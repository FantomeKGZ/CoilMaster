/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Buffer.h
Module    : Shared/Protocol

Description:
Ring Buffer
==========================================================
*/

#ifndef CMP_BUFFER_H
#define CMP_BUFFER_H

#include <stdint.h>

#include "CMP_Defines.h"

class CMP_Buffer
{
public:

    CMP_Buffer();

    //------------------------------------------------------
    // Control
    //------------------------------------------------------

    void clear();

    //------------------------------------------------------
    // Write
    //------------------------------------------------------

    bool push(uint8_t value);

    //------------------------------------------------------
    // Read
    //------------------------------------------------------

    bool pop(uint8_t& value);

    //------------------------------------------------------
    // Peek
    //------------------------------------------------------

    bool peek(uint16_t index,
              uint8_t& value) const;

    bool peek(uint8_t* data,
              uint16_t length) const;

    //------------------------------------------------------
    // Read block
    //------------------------------------------------------

    bool read(uint8_t* data,
              uint16_t length);

    //------------------------------------------------------
    // Skip bytes
    //------------------------------------------------------

    bool discard(uint16_t count);

    //------------------------------------------------------
    // Information
    //------------------------------------------------------

    uint16_t available() const;

    uint16_t size() const;

    bool empty() const;

    bool full() const;

private:

    uint8_t buffer[CMP_RX_BUFFER_SIZE];

    uint16_t head;

    uint16_t tail;

    uint16_t count;
};

#endif