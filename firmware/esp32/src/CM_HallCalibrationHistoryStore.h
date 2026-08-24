#ifndef CM_HALL_CALIBRATION_HISTORY_STORE_H
#define CM_HALL_CALIBRATION_HISTORY_STORE_H

#include <Arduino.h>
#include <FS.h>

#include "CM_HardwareControlClient.h"
#include "CM_RtcClock.h"

namespace CM
{

struct HallCalibrationHistoryEntry
{
    uint32_t measurementId = 0UL;
    uint32_t recordedAtMs = 0UL;
    bool rtcValid = false;
    RtcDateTime recordedAt;

    uint16_t baselineAdc = 0U;
    uint16_t minAdc = 0U;
    uint16_t maxAdc = 0U;
    uint16_t sampleCount = 0U;
    uint32_t durationMs = 0UL;

    bool recommendationValid = false;
    uint16_t recommendedThreshold = 0U;
    uint16_t recommendedHysteresis = 0U;
    HallSignalDirectionRemote recommendedDirection = HallSignalDirectionRemote::Rising;

    HardwareControlReplyResult applyResult = HardwareControlReplyResult::None;
    bool persistedProfileValid = false;
    uint16_t persistedThreshold = 0U;
    uint16_t persistedHysteresis = 0U;
    uint16_t persistedReleaseDebounceMs = 0U;
    HallSignalDirectionRemote persistedDirection = HallSignalDirectionRemote::Rising;
};

class HallCalibrationHistoryStore
{
public:
    static constexpr uint8_t MaxEntries = 10U;

    explicit HallCalibrationHistoryStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool recordMeasurement(const HallCalibrationRemoteResult& result,
                           uint32_t nowMs,
                           const RtcDateTime* recordedAt);
    bool finalize(uint32_t measurementId,
                  HardwareControlReplyResult result,
                  const HallSettingsState* persistedProfile);
    bool load(HallCalibrationHistoryEntry* entries, uint8_t& count) const;

private:
    static constexpr const char* Directory = "/data/hardware";
    static constexpr const char* HistoryPath =
        "/data/hardware/hall-calibration-history.ndjson";
    static constexpr const char* TempPath =
        "/data/hardware/hall-calibration-history.tmp";
    static constexpr const char* BackupPath =
        "/data/hardware/hall-calibration-history.bak";

    bool recoverFileSwap();
    bool loadFromPath(const char* path,
                      HallCalibrationHistoryEntry* entries,
                      uint8_t& count) const;
    bool saveAll(const HallCalibrationHistoryEntry* entries, uint8_t count);
    static bool valid(const HallCalibrationHistoryEntry& entry);

    fs::FS& m_storage;
    bool m_ready;
};

} // namespace CM

#endif // CM_HALL_CALIBRATION_HISTORY_STORE_H
