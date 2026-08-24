#include "CM_HallCalibrationProtocol.h"

#include <stdio.h>
#include <avr/pgmspace.h>

#include "../Shared/CMP1Text/CM_Cmp1Crc.h"

namespace CM
{
namespace HallCalibrationProtocol
{

bool formatDone(const HallCalibrationResult& result,
                char* output,
                size_t outputSize)
{
    if (output == nullptr || outputSize == 0U || result.measurementId == 0UL)
        return false;

    const int payloadLength = snprintf_P(
        output,
        outputSize,
        PSTR("CMP1|CAL_DONE|%lu|C"),
        static_cast<unsigned long>(result.measurementId));
    if (payloadLength <= 0 || static_cast<size_t>(payloadLength) >= outputSize)
        return false;

    const uint16_t crc = Cmp1Crc::calculate(
        reinterpret_cast<const uint8_t*>(output),
        static_cast<size_t>(payloadLength));
    const int suffixLength = snprintf_P(
        output + payloadLength,
        outputSize - static_cast<size_t>(payloadLength),
        PSTR("|%04X\n"),
        static_cast<unsigned int>(crc));

    return suffixLength > 0 &&
           static_cast<size_t>(payloadLength + suffixLength) < outputSize;
}

} // namespace HallCalibrationProtocol
} // namespace CM
