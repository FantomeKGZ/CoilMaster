#ifndef CM_CONDUCTOR_SETTINGS_H
#define CM_CONDUCTOR_SETTINGS_H

#include <Arduino.h>
#include <FS.h>

#include "CM_ConductorCalculator.h"

namespace CM
{
class ConductorSettingsStore
{
public:
    explicit ConductorSettingsStore(fs::FS& storage);

    bool begin();
    bool load(ConversionSettings& settings) const;
    bool save(const ConversionSettings& settings);
    bool ready() const;

private:
    static constexpr const char* SettingsPath =
        "/data/settings/conductor-calculator.ndjson";
    static constexpr const char* TempPath =
        "/data/settings/conductor-calculator.tmp";

    static bool findUnsigned(const String& line,
                             const char* key,
                             uint32_t& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_CONDUCTOR_SETTINGS_H
