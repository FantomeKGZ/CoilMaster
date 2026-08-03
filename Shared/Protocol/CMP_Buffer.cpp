/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Buffer.cpp
==========================================================
*/

#include "CMP_Buffer.h"

CMP_Buffer::CMP_Buffer()
{
    clear();
}

void CMP_Buffer::clear()
{
    head = 0;
    tail = 0;
    count = 0;
}

bool CMP_Buffer::push(uint8_t value)
{
    if (full())
        return false;

    buffer[head] = value;

    head++;

    if (head >= CMP_RX_BUFFER_SIZE)
        head = 0;

    count++;

    return true;
}

bool CMP_Buffer::pop(uint8_t& value)
{
    if (empty())
        return false;

    value = buffer[tail];

    tail++;

    if (tail >= CMP_RX_BUFFER_SIZE)
        tail = 0;

    count--;

    return true;
}

bool CMP_Buffer::peek(uint16_t index,
                      uint8_t& value) const
{
    if (index >= count)
        return false;

    uint16_t pos = tail + index;

    if (pos >= CMP_RX_BUFFER_SIZE)
        pos -= CMP_RX_BUFFER_SIZE;

    value = buffer[pos];

    return true;
}

bool CMP_Buffer::peek(uint8_t* data,
                      uint16_t length) const
{
    if (length > count)
        return false;

    for (uint16_t i = 0; i < length; i++)
    {
        if (!peek(i, data[i]))
            return false;
    }

    return true;
}

bool CMP_Buffer::read(uint8_t* data,
                      uint16_t length)
{
    if (length > count)
        return false;

    for (uint16_t i = 0; i < length; i++)
    {
        pop(data[i]);
    }

    return true;
}

bool CMP_Buffer::discard(uint16_t length)
{
    if (length > count)
        return false;

    uint8_t dummy;

    while (length--)
    {
        pop(dummy);
    }

    return true;
}

uint16_t CMP_Buffer::available() const
{
    return count;
}

uint16_t CMP_Buffer::size() const
{
    return count;
}

bool CMP_Buffer::empty() const
{
    return count == 0;
}

bool CMP_Buffer::full() const
{
    return count >= CMP_RX_BUFFER_SIZE;
}