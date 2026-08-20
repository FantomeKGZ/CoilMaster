#ifndef CM_HARDWARE_SETTINGS_CONTROLLER_H
#define CM_HARDWARE_SETTINGS_CONTROLLER_H

#include <Arduino.h>

#include "CM_HallTurnSource.h"
#include "CM_HardwareSettings.h"
#include "../Core/CM_StateMachine.h"

namespace CM
{

enum class HardwareSettingsApplyResult : uint8_t
{
    Applied = 0U,
    Invalid,
    Busy,
    PersistenceFailed
};

class HardwareSettingsController
{
public:
    HardwareSettingsController(HardwareSettingsStore& store,
                               HallTurnSource& hall,
                               StateMachine& machine);

    void begin();

    const HardwareSettings& settings() const;
    bool loadedFromEeprom() const;
    bool usedFactoryFallback() const;

    bool safeToChange() const;
    HardwareSettingsApplyResult apply(const HardwareSettings& settings);
    HardwareSettingsApplyResult resetToFactoryDefaults();

private:
    void applyToHall(const HardwareSettings& settings);

    HardwareSettingsStore& m_store;
    HallTurnSource& m_hall;
    StateMachine& m_machine;
};

} // namespace CM

#endif // CM_HARDWARE_SETTINGS_CONTROLLER_H
