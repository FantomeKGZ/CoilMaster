#include <stddef.h>
#include <stdint.h>

namespace
{
inline uint8_t hexDigit(char value)
{
    if (value >= '0' && value <= '9')
        return static_cast<uint8_t>(value - '0');
    if (value >= 'A' && value <= 'F')
        return static_cast<uint8_t>(value - 'A' + 10);
    if (value >= 'a' && value <= 'f')
        return static_cast<uint8_t>(value - 'a' + 10);
    return 0xFFU;
}
}

extern "C" unsigned long cm_strtoul(const char* text, char** end, int base)
{
    if (end != nullptr) *end = const_cast<char*>(text);
    if (text == nullptr || base != 16) return 0UL;

    unsigned long value = 0UL;
    const char* cursor = text;
    for (;;)
    {
        const uint8_t digit = hexDigit(*cursor);
        if (digit > 0x0FU) break;
        value = (value << 4U) | digit;
        ++cursor;
    }

    if (cursor == text) return 0UL;
    if (end != nullptr) *end = const_cast<char*>(cursor);
    return value;
}
