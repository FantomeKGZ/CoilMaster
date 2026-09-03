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

    uint16_t value = 0U;
    for (uint8_t index = 0U; index < 4U; ++index)
    {
        const uint8_t digit = hexDigit(text[index]);
        if (digit > 0x0FU)
        {
            if (end != nullptr) *end = const_cast<char*>(text + index);
            return value;
        }
        value = static_cast<uint16_t>((value << 4U) | digit);
    }

    if (end != nullptr) *end = const_cast<char*>(text + 4U);
    return value;
}
