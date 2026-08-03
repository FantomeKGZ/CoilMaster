#include "CMP_Buffer.h"

namespace CMP
{
Buffer::Buffer()
    : m_head(0U),
      m_tail(0U),
      m_count(0U)
{
}

void Buffer::clear()
{
    m_head = 0U;
    m_tail = 0U;
    m_count = 0U;
}

Result Buffer::push(uint8_t value)
{
    if (full())
    {
        return Result::BufferFull;
    }

    m_data[m_head] = value;
    ++m_head;

    if (m_head >= Capacity)
    {
        m_head = 0U;
    }

    ++m_count;
    return Result::Ok;
}

Result Buffer::push(const uint8_t* data, uint16_t length)
{
    if ((data == nullptr) && (length > 0U))
    {
        return Result::InvalidArgument;
    }

    if (freeSpace() < length)
    {
        return Result::BufferFull;
    }

    for (uint16_t index = 0U; index < length; ++index)
    {
        const Result result = push(data[index]);
        if (result != Result::Ok)
        {
            return result;
        }
    }

    return Result::Ok;
}

Result Buffer::pop(uint8_t& value)
{
    if (empty())
    {
        return Result::BufferEmpty;
    }

    value = m_data[m_tail];
    ++m_tail;

    if (m_tail >= Capacity)
    {
        m_tail = 0U;
    }

    --m_count;
    return Result::Ok;
}

Result Buffer::pop(uint8_t* data, uint16_t length)
{
    if ((data == nullptr) && (length > 0U))
    {
        return Result::InvalidArgument;
    }

    if (!contains(length))
    {
        return Result::BufferEmpty;
    }

    for (uint16_t index = 0U; index < length; ++index)
    {
        const Result result = pop(data[index]);
        if (result != Result::Ok)
        {
            return result;
        }
    }

    return Result::Ok;
}

Result Buffer::peek(uint16_t offset, uint8_t& value) const
{
    if (offset >= m_count)
    {
        return Result::BufferEmpty;
    }

    uint16_t position = static_cast<uint16_t>(m_tail + offset);
    if (position >= Capacity)
    {
        position = static_cast<uint16_t>(position - Capacity);
    }

    value = m_data[position];
    return Result::Ok;
}

Result Buffer::peek(uint16_t offset,
                    uint8_t* data,
                    uint16_t length) const
{
    if ((data == nullptr) && (length > 0U))
    {
        return Result::InvalidArgument;
    }

    if ((offset > m_count) || (length > static_cast<uint16_t>(m_count - offset)))
    {
        return Result::BufferEmpty;
    }

    for (uint16_t index = 0U; index < length; ++index)
    {
        const Result result = peek(static_cast<uint16_t>(offset + index),
                                   data[index]);
        if (result != Result::Ok)
        {
            return result;
        }
    }

    return Result::Ok;
}

Result Buffer::discard(uint16_t length)
{
    if (!contains(length))
    {
        return Result::BufferEmpty;
    }

    m_tail = static_cast<uint16_t>(m_tail + length);
    while (m_tail >= Capacity)
    {
        m_tail = static_cast<uint16_t>(m_tail - Capacity);
    }

    m_count = static_cast<uint16_t>(m_count - length);
    return Result::Ok;
}

bool Buffer::empty() const
{
    return m_count == 0U;
}

bool Buffer::full() const
{
    return m_count == Capacity;
}

bool Buffer::contains(uint16_t length) const
{
    return m_count >= length;
}

uint16_t Buffer::size() const
{
    return m_count;
}

uint16_t Buffer::freeSpace() const
{
    return static_cast<uint16_t>(Capacity - m_count);
}

uint16_t Buffer::capacity() const
{
    return Capacity;
}
}
