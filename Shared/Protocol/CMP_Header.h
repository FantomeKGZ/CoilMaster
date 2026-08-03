#ifndef CMP_HEADER_H
#define CMP_HEADER_H

#include <stdint.h>

#include "CMP_Command.h"
#include "CMP_Flags.h"

namespace CMP
{
struct Header
{
    uint16_t startWord;
    uint8_t versionMajor;
    uint8_t versionMinor;
    Flags flags;
    uint8_t reserved;
    Command command;
    uint16_t counter;
    uint16_t payloadLength;
};
}

#endif // CMP_HEADER_H
