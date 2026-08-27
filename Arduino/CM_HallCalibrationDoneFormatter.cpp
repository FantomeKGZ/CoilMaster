#include "CM_HallCalibrationProtocol.h"

#include <stdio.h>
#include <avr/pgmspace.h>

#include "CM_CrcFrameText.h"

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

    return CrcFrameText::append(
        output, outputSize, static_cast<size_t>(payloadLength));
}

} // namespace HallCalibrationProtocol
} // namespace CM
