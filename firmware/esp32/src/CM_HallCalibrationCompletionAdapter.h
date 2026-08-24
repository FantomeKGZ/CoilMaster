#ifndef CM_HALL_CALIBRATION_COMPLETION_ADAPTER_H
#define CM_HALL_CALIBRATION_COMPLETION_ADAPTER_H

#include <Arduino.h>

#include "CM_HallCalibrationDoneProtocol.h"
#include "CM_HallCalibrationRawCollector.h"
#include "CM_HardwareControlClient.h"

namespace CM
{
namespace HallCalibrationCompletionAdapter
{
// Build the same remote-result shape used by the Web/analyzer path from the
// compact Uno-owned CAL_DONE correlation token plus ESP32-owned raw summary.
// This layer carries no actuator semantics and never controls START/SSR.
bool buildFromDone(const HallCalibrationDone& done,
                   HallCalibrationRawCollector& collector,
                   bool rawRunStarted,
                   uint32_t rawDurationMs,
                   uint32_t nowMs,
                   HallCalibrationRemoteResult& result);

// Keep legacy CAL_RESULT backward compatible while raw aggregation remains the
// authoritative source of baseline/min/max/sample count/duration on ESP32.
bool enrichLegacy(HallCalibrationRemoteResult& result,
                  HallCalibrationRawCollector& collector,
                  bool rawRunStarted,
                  uint32_t rawDurationMs);
}
} // namespace CM

#endif // CM_HALL_CALIBRATION_COMPLETION_ADAPTER_H
