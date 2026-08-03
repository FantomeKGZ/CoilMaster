#ifndef CMP_BUFFER_H
#define CMP_BUFFER_H

#include <stdint.h>

#include "CMP_Defines.h"
#include "CMP_Result.h"

namespace CMP
{
class Buffer
{
public:
    Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void clear();

    Result push(uint8_t value);
    Result push(const uint8_t* data, uint16_t length);

    Result pop(uint8_t& value);
    Result pop(uint8_t* data, uint16_t length);

    Result peek(uint16_t offset, uint8_t& value) const;
    Result peek(uint16_t offset,
                uint8_t* data,
                uint16_t length) const;

    Result discard(uint16_t length);

    bool empty() const;
    bool full() const;
    bool contains(uint16_t length) const;

    uint16_t size() const;
    uint16_t freeSpace() const;
    uint16_t capacity() const;

private:
    static constexpr uint16_t Capacity = RxBufferSize;

    uint8_t m_data[Capacity];
    uint16_t m_head;
    uint16_t m_tail;
    uint16_t m_count;
};
}

#endif // CMP_BUFFER_H
