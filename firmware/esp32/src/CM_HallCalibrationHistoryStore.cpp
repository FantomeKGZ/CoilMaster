#include "CM_HallCalibrationHistoryStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
constexpr uint32_t SchemaVersion = 1UL;

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    value = 0UL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    int cursor = start + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    uint32_t parsed = 0UL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    if (cursor >= line.length() ||
        (line[cursor] != ',' && line[cursor] != '}')) return false;
    value = parsed;
    return true;
}

bool findBoolean(const String& line, const char* key, bool& value)
{
    value = false;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    int cursor = start + marker.length();
    if (line.substring(cursor, cursor + 4) == "true")
    {
        value = true;
        cursor += 4;
    }
    else if (line.substring(cursor, cursor + 5) == "false")
    {
        cursor += 5;
    }
    else return false;
    return cursor < line.length() &&
           (line[cursor] == ',' || line[cursor] == '}');
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0)
        return false;
    const int valueStart = start + marker.length();
    const int valueEnd = line.indexOf('"', valueStart);
    if (valueEnd < 0 || valueEnd + 1 >= line.length() ||
        (line[valueEnd + 1] != ',' && line[valueEnd + 1] != '}'))
        return false;
    value = line.substring(valueStart, valueEnd);
    return true;
}

const char* directionText(HallSignalDirectionRemote direction)
{
    return direction == HallSignalDirectionRemote::Falling ? "FALLING" : "RISING";
}

bool parseDirection(const String& text, HallSignalDirectionRemote& direction)
{
    if (text == "RISING")
    {
        direction = HallSignalDirectionRemote::Rising;
        return true;
    }
    if (text == "FALLING")
    {
        direction = HallSignalDirectionRemote::Falling;
        return true;
    }
    return false;
}

const char* resultText(HardwareControlReplyResult result)
{
    switch (result)
    {
        case HardwareControlReplyResult::Applied: return "APPLIED";
        case HardwareControlReplyResult::Busy: return "BUSY";
        case HardwareControlReplyResult::Invalid: return "INVALID";
        case HardwareControlReplyResult::IdentityMismatch: return "IDENTITY_MISMATCH";
        case HardwareControlReplyResult::Cancelled: return "CANCELLED";
        case HardwareControlReplyResult::PersistenceFailed: return "PERSISTENCE_FAILED";
        case HardwareControlReplyResult::Unsupported: return "UNSUPPORTED";
        case HardwareControlReplyResult::TimedOut: return "TIMED_OUT";
        case HardwareControlReplyResult::None:
        default: return "NONE";
    }
}

bool parseResult(const String& text, HardwareControlReplyResult& result)
{
    if (text == "NONE") result = HardwareControlReplyResult::None;
    else if (text == "APPLIED") result = HardwareControlReplyResult::Applied;
    else if (text == "BUSY") result = HardwareControlReplyResult::Busy;
    else if (text == "INVALID") result = HardwareControlReplyResult::Invalid;
    else if (text == "IDENTITY_MISMATCH") result = HardwareControlReplyResult::IdentityMismatch;
    else if (text == "CANCELLED") result = HardwareControlReplyResult::Cancelled;
    else if (text == "PERSISTENCE_FAILED") result = HardwareControlReplyResult::PersistenceFailed;
    else if (text == "UNSUPPORTED") result = HardwareControlReplyResult::Unsupported;
    else if (text == "TIMED_OUT") result = HardwareControlReplyResult::TimedOut;
    else return false;
    return true;
}
}

HallCalibrationHistoryStore::HallCalibrationHistoryStore(fs::FS& storage)
    : m_storage(storage), m_ready(false)
{
}

bool HallCalibrationHistoryStore::begin()
{
    m_ready = false;
    if (!m_storage.exists("/data") && !m_storage.mkdir("/data")) return false;
    if (!m_storage.exists(Directory) && !m_storage.mkdir(Directory)) return false;
    if (!recoverFileSwap()) return false;
    if (m_storage.exists(HistoryPath))
    {
        HallCalibrationHistoryEntry entries[MaxEntries];
        uint8_t count = 0U;
        if (!loadFromPath(HistoryPath, entries, count)) return false;
    }
    m_ready = true;
    return true;
}

bool HallCalibrationHistoryStore::ready() const
{
    if (!m_ready) return false;
    File directory = m_storage.open(Directory, FILE_READ);
    if (!directory) return false;
    const bool available = directory.isDirectory();
    directory.close();
    return available;
}

bool HallCalibrationHistoryStore::recordMeasurement(
    const HallCalibrationRemoteResult& result,
    uint32_t nowMs,
    const RtcDateTime* recordedAt)
{
    if (!ready() || !result.valid || result.measurementId == 0UL) return false;

    HallCalibrationHistoryEntry entries[MaxEntries];
    uint8_t count = 0U;
    if (!load(entries, count)) return false;

    HallCalibrationHistoryEntry entry;
    entry.measurementId = result.measurementId;
    entry.recordedAtMs = nowMs;
    if (recordedAt != nullptr)
    {
        entry.rtcValid = true;
        entry.recordedAt = *recordedAt;
    }
    entry.baselineAdc = result.baselineAdc;
    entry.minAdc = result.minAdc;
    entry.maxAdc = result.maxAdc;
    entry.sampleCount = result.sampleCount;
    entry.durationMs = result.durationMs;
    entry.recommendationValid = result.recommendationValid;
    entry.recommendedThreshold = result.recommendedThreshold;
    entry.recommendedHysteresis = result.recommendedHysteresis;
    entry.recommendedDirection = result.direction;
    if (!valid(entry)) return false;

    int existing = -1;
    for (uint8_t i = 0U; i < count; ++i)
        if (entries[i].measurementId == entry.measurementId) existing = i;

    if (existing >= 0)
    {
        const HardwareControlReplyResult previousResult = entries[existing].applyResult;
        const bool previousPersisted = entries[existing].persistedProfileValid;
        const uint16_t persistedThreshold = entries[existing].persistedThreshold;
        const uint16_t persistedHysteresis = entries[existing].persistedHysteresis;
        const uint16_t persistedDebounce = entries[existing].persistedReleaseDebounceMs;
        const HallSignalDirectionRemote persistedDirection = entries[existing].persistedDirection;
        entries[existing] = entry;
        entries[existing].applyResult = previousResult;
        entries[existing].persistedProfileValid = previousPersisted;
        entries[existing].persistedThreshold = persistedThreshold;
        entries[existing].persistedHysteresis = persistedHysteresis;
        entries[existing].persistedReleaseDebounceMs = persistedDebounce;
        entries[existing].persistedDirection = persistedDirection;
    }
    else if (count < MaxEntries)
    {
        entries[count++] = entry;
    }
    else
    {
        for (uint8_t i = 1U; i < MaxEntries; ++i) entries[i - 1U] = entries[i];
        entries[MaxEntries - 1U] = entry;
    }
    return saveAll(entries, count);
}

bool HallCalibrationHistoryStore::finalize(
    uint32_t measurementId,
    HardwareControlReplyResult result,
    const HallSettingsState* persistedProfile)
{
    if (!ready() || measurementId == 0UL || result == HardwareControlReplyResult::None)
        return false;

    HallCalibrationHistoryEntry entries[MaxEntries];
    uint8_t count = 0U;
    if (!load(entries, count)) return false;
    for (uint8_t i = 0U; i < count; ++i)
    {
        if (entries[i].measurementId != measurementId) continue;
        entries[i].applyResult = result;
        entries[i].persistedProfileValid = false;
        entries[i].persistedThreshold = 0U;
        entries[i].persistedHysteresis = 0U;
        entries[i].persistedReleaseDebounceMs = 0U;
        entries[i].persistedDirection = HallSignalDirectionRemote::Rising;
        if (result == HardwareControlReplyResult::Applied && persistedProfile != nullptr &&
            persistedProfile->valid && persistedProfile->fromEeprom)
        {
            entries[i].persistedProfileValid = true;
            entries[i].persistedThreshold = persistedProfile->threshold;
            entries[i].persistedHysteresis = persistedProfile->hysteresis;
            entries[i].persistedReleaseDebounceMs = persistedProfile->releaseDebounceMs;
            entries[i].persistedDirection = persistedProfile->direction;
        }
        if (!valid(entries[i])) return false;
        return saveAll(entries, count);
    }
    return false;
}

bool HallCalibrationHistoryStore::load(HallCalibrationHistoryEntry* entries,
                                       uint8_t& count) const
{
    count = 0U;
    if (!ready() || entries == nullptr) return false;
    if (!m_storage.exists(HistoryPath)) return true;
    return loadFromPath(HistoryPath, entries, count);
}

bool HallCalibrationHistoryStore::valid(const HallCalibrationHistoryEntry& entry)
{
    if (entry.measurementId == 0UL || entry.baselineAdc > 1023U ||
        entry.minAdc > 1023U || entry.maxAdc > 1023U || entry.minAdc > entry.maxAdc)
        return false;
    if (entry.recommendationValid &&
        (entry.recommendedThreshold == 0U || entry.recommendedThreshold > 1023U ||
         entry.recommendedHysteresis == 0U || entry.recommendedHysteresis > 512U ||
         entry.recommendedHysteresis >= entry.recommendedThreshold))
        return false;
    if (entry.persistedProfileValid &&
        (entry.persistedThreshold == 0U || entry.persistedThreshold > 1023U ||
         entry.persistedHysteresis == 0U || entry.persistedHysteresis > 512U ||
         entry.persistedHysteresis >= entry.persistedThreshold ||
         entry.persistedReleaseDebounceMs == 0U ||
         entry.persistedReleaseDebounceMs > 1000U))
        return false;
    if (entry.rtcValid &&
        (entry.recordedAt.year < 2000U || entry.recordedAt.year > 2099U ||
         entry.recordedAt.month < 1U || entry.recordedAt.month > 12U ||
         entry.recordedAt.day < 1U || entry.recordedAt.day > 31U ||
         entry.recordedAt.hour > 23U || entry.recordedAt.minute > 59U ||
         entry.recordedAt.second > 59U))
        return false;
    return true;
}

bool HallCalibrationHistoryStore::recoverFileSwap()
{
    const bool mainExists = m_storage.exists(HistoryPath);
    const bool tempExists = m_storage.exists(TempPath);
    const bool backupExists = m_storage.exists(BackupPath);
    if (!tempExists && !backupExists) return true;

    HallCalibrationHistoryEntry entries[MaxEntries];
    uint8_t count = 0U;
    if (mainExists && loadFromPath(HistoryPath, entries, count))
    {
        if (tempExists && !m_storage.remove(TempPath)) return false;
        if (backupExists && !m_storage.remove(BackupPath)) return false;
        return true;
    }

    HallCalibrationHistoryEntry tempEntries[MaxEntries];
    HallCalibrationHistoryEntry backupEntries[MaxEntries];
    uint8_t tempCount = 0U, backupCount = 0U;
    const bool tempValid = tempExists && loadFromPath(TempPath, tempEntries, tempCount);
    const bool backupValid = backupExists && loadFromPath(BackupPath, backupEntries, backupCount);

    if (backupValid)
    {
        if (mainExists && !m_storage.remove(HistoryPath)) return false;
        if (tempExists && !m_storage.remove(TempPath)) return false;
        return m_storage.rename(BackupPath, HistoryPath);
    }
    if (backupExists) return false;
    if (tempValid)
    {
        if (mainExists && !m_storage.remove(HistoryPath)) return false;
        return m_storage.rename(TempPath, HistoryPath);
    }
    return false;
}

bool HallCalibrationHistoryStore::loadFromPath(
    const char* path,
    HallCalibrationHistoryEntry* entries,
    uint8_t& count) const
{
    count = 0U;
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (count >= MaxEntries || !FlatJsonObjectValidator::valid(line))
        {
            file.close(); count = 0U; return false;
        }

        HallCalibrationHistoryEntry entry;
        uint32_t schema = 0UL, rtcValid = 0UL, recommendationValid = 0UL,
                 persistedValid = 0UL;
        uint32_t year = 0UL, month = 0UL, day = 0UL, hour = 0UL,
                 minute = 0UL, second = 0UL;
        uint32_t baseline = 0UL, minAdc = 0UL, maxAdc = 0UL, samples = 0UL;
        uint32_t recommendedThreshold = 0UL, recommendedHysteresis = 0UL;
        uint32_t persistedThreshold = 0UL, persistedHysteresis = 0UL,
                 persistedDebounce = 0UL;
        bool rtcFlag = false, recommendationFlag = false, persistedFlag = false;
        String recommendedDirection, persistedDirection, applyResult;

        if (!findUnsigned(line, "schema", schema) || schema != SchemaVersion ||
            !findUnsigned(line, "measurement_id", entry.measurementId) ||
            !findUnsigned(line, "recorded_at_ms", entry.recordedAtMs) ||
            !findBoolean(line, "rtc_valid", rtcFlag) ||
            !findUnsigned(line, "year", year) || !findUnsigned(line, "month", month) ||
            !findUnsigned(line, "day", day) || !findUnsigned(line, "hour", hour) ||
            !findUnsigned(line, "minute", minute) || !findUnsigned(line, "second", second) ||
            !findUnsigned(line, "baseline", baseline) || !findUnsigned(line, "min", minAdc) ||
            !findUnsigned(line, "max", maxAdc) || !findUnsigned(line, "samples", samples) ||
            !findUnsigned(line, "duration_ms", entry.durationMs) ||
            !findBoolean(line, "recommendation_valid", recommendationFlag) ||
            !findUnsigned(line, "recommended_threshold", recommendedThreshold) ||
            !findUnsigned(line, "recommended_hysteresis", recommendedHysteresis) ||
            !findString(line, "recommended_direction", recommendedDirection) ||
            !findString(line, "apply_result", applyResult) ||
            !findBoolean(line, "persisted_valid", persistedFlag) ||
            !findUnsigned(line, "persisted_threshold", persistedThreshold) ||
            !findUnsigned(line, "persisted_hysteresis", persistedHysteresis) ||
            !findUnsigned(line, "persisted_release_debounce_ms", persistedDebounce) ||
            !findString(line, "persisted_direction", persistedDirection) ||
            !parseDirection(recommendedDirection, entry.recommendedDirection) ||
            !parseDirection(persistedDirection, entry.persistedDirection) ||
            !parseResult(applyResult, entry.applyResult))
        {
            file.close(); count = 0U; return false;
        }

        (void)rtcValid; (void)recommendationValid; (void)persistedValid;
        entry.rtcValid = rtcFlag;
        entry.recordedAt.year = static_cast<uint16_t>(year);
        entry.recordedAt.month = static_cast<uint8_t>(month);
        entry.recordedAt.day = static_cast<uint8_t>(day);
        entry.recordedAt.hour = static_cast<uint8_t>(hour);
        entry.recordedAt.minute = static_cast<uint8_t>(minute);
        entry.recordedAt.second = static_cast<uint8_t>(second);
        entry.baselineAdc = static_cast<uint16_t>(baseline);
        entry.minAdc = static_cast<uint16_t>(minAdc);
        entry.maxAdc = static_cast<uint16_t>(maxAdc);
        entry.sampleCount = static_cast<uint16_t>(samples);
        entry.recommendationValid = recommendationFlag;
        entry.recommendedThreshold = static_cast<uint16_t>(recommendedThreshold);
        entry.recommendedHysteresis = static_cast<uint16_t>(recommendedHysteresis);
        entry.persistedProfileValid = persistedFlag;
        entry.persistedThreshold = static_cast<uint16_t>(persistedThreshold);
        entry.persistedHysteresis = static_cast<uint16_t>(persistedHysteresis);
        entry.persistedReleaseDebounceMs = static_cast<uint16_t>(persistedDebounce);

        if (year > 0xFFFFUL || month > 0xFFUL || day > 0xFFUL || hour > 0xFFUL ||
            minute > 0xFFUL || second > 0xFFUL || baseline > 0xFFFFUL ||
            minAdc > 0xFFFFUL || maxAdc > 0xFFFFUL || samples > 0xFFFFUL ||
            recommendedThreshold > 0xFFFFUL || recommendedHysteresis > 0xFFFFUL ||
            persistedThreshold > 0xFFFFUL || persistedHysteresis > 0xFFFFUL ||
            persistedDebounce > 0xFFFFUL || !valid(entry))
        {
            file.close(); count = 0U; return false;
        }
        for (uint8_t i = 0U; i < count; ++i)
            if (entries[i].measurementId == entry.measurementId)
            {
                file.close(); count = 0U; return false;
            }
        entries[count++] = entry;
    }
    file.close();
    return true;
}

bool HallCalibrationHistoryStore::saveAll(
    const HallCalibrationHistoryEntry* entries,
    uint8_t count)
{
    if (!ready() || entries == nullptr || count > MaxEntries) return false;
    if (m_storage.exists(TempPath) && !m_storage.remove(TempPath)) return false;
    File file = m_storage.open(TempPath, FILE_WRITE);
    if (!file) return false;

    bool written = true;
    for (uint8_t i = 0U; i < count; ++i)
    {
        const HallCalibrationHistoryEntry& entry = entries[i];
        if (!valid(entry)) { written = false; break; }
        String line;
        line.reserve(560U);
        line += F("{\"schema\":1,\"measurement_id\":"); line += entry.measurementId;
        line += F(",\"recorded_at_ms\":"); line += entry.recordedAtMs;
        line += F(",\"rtc_valid\":"); line += entry.rtcValid ? F("true") : F("false");
        line += F(",\"year\":"); line += entry.recordedAt.year;
        line += F(",\"month\":"); line += entry.recordedAt.month;
        line += F(",\"day\":"); line += entry.recordedAt.day;
        line += F(",\"hour\":"); line += entry.recordedAt.hour;
        line += F(",\"minute\":"); line += entry.recordedAt.minute;
        line += F(",\"second\":"); line += entry.recordedAt.second;
        line += F(",\"baseline\":"); line += entry.baselineAdc;
        line += F(",\"min\":"); line += entry.minAdc;
        line += F(",\"max\":"); line += entry.maxAdc;
        line += F(",\"samples\":"); line += entry.sampleCount;
        line += F(",\"duration_ms\":"); line += entry.durationMs;
        line += F(",\"recommendation_valid\":");
        line += entry.recommendationValid ? F("true") : F("false");
        line += F(",\"recommended_threshold\":"); line += entry.recommendedThreshold;
        line += F(",\"recommended_hysteresis\":"); line += entry.recommendedHysteresis;
        line += F(",\"recommended_direction\":\""); line += directionText(entry.recommendedDirection);
        line += F("\",\"apply_result\":\""); line += resultText(entry.applyResult);
        line += F("\",\"persisted_valid\":");
        line += entry.persistedProfileValid ? F("true") : F("false");
        line += F(",\"persisted_threshold\":"); line += entry.persistedThreshold;
        line += F(",\"persisted_hysteresis\":"); line += entry.persistedHysteresis;
        line += F(",\"persisted_release_debounce_ms\":");
        line += entry.persistedReleaseDebounceMs;
        line += F(",\"persisted_direction\":\""); line += directionText(entry.persistedDirection);
        line += F("\"}\n");
        if (file.print(line) != line.length()) { written = false; break; }
    }
    file.flush();
    file.close();

    HallCalibrationHistoryEntry verified[MaxEntries];
    uint8_t verifiedCount = 0U;
    if (!written || !loadFromPath(TempPath, verified, verifiedCount) ||
        verifiedCount != count)
    {
        m_storage.remove(TempPath);
        return false;
    }
    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    {
        m_storage.remove(TempPath); return false;
    }
    if (m_storage.exists(HistoryPath) && !m_storage.rename(HistoryPath, BackupPath))
    {
        m_storage.remove(TempPath); return false;
    }
    if (!m_storage.rename(TempPath, HistoryPath))
    {
        if (!m_storage.exists(HistoryPath) && m_storage.exists(BackupPath))
            m_storage.rename(BackupPath, HistoryPath);
        return false;
    }
    if (m_storage.exists(BackupPath) && !m_storage.remove(BackupPath))
    {
        m_ready = false;
        return false;
    }
    return true;
}

} // namespace CM
