#include "CM_HallCalibrationCompletionAdapter.h"

namespace CM
{
namespace HallCalibrationCompletionAdapter
{
namespace
{
bool applyRawSummary(HallCalibrationRawCollector& collector,
                     bool rawRunStarted,
                     uint32_t rawDurationMs,
                     HallCalibrationRemoteResult& result)
{
    if (!rawRunStarted || !collector.finish(rawDurationMs)) return false;

    const HallCalibrationRawSummary summary = collector.summary();
    if (!summary.valid) return false;

    result.baselineAdc = summary.baselineAdc;
    result.minAdc = summary.minAdc;
    result.maxAdc = summary.maxAdc;
    result.sampleCount = summary.runSamples;
    result.durationMs = summary.durationMs;
    return true;
}
}

bool buildFromDone(const HallCalibrationDone& done,
                   HallCalibrationRawCollector& collector,
                   bool rawRunStarted,
                   uint32_t rawDurationMs,
                   uint32_t nowMs,
                   HallCalibrationRemoteResult& result)
{
    result = HallCalibrationRemoteResult();
    if (!done.valid || done.measurementId == 0UL) return false;

    result.measurementId = done.measurementId;
    result.recommendationValid = false;
    result.receivedAtMs = nowMs;
    if (!applyRawSummary(collector, rawRunStarted, rawDurationMs, result))
        return false;

    result.valid = true;
    return true;
}

bool enrichLegacy(HallCalibrationRemoteResult& result,
                  HallCalibrationRawCollector& collector,
                  bool rawRunStarted,
                  uint32_t rawDurationMs)
{
    if (!result.valid || result.measurementId == 0UL) return false;
    return applyRawSummary(collector, rawRunStarted, rawDurationMs, result);
}

} // namespace HallCalibrationCompletionAdapter
} // namespace CM
