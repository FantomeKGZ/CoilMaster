#include "CM_HardwareSettingsController.h"

namespace CM
{

HardwareSettingsController::HardwareSettingsController(
    HardwareSettingsStore& store,
    HallTurnSource& hall,
    StateMachine& machine)
    : m_store(store),
      m_hall(hall),
      m_machine(machine)
{
}

void HardwareSettingsController::begin()
{
    m_store.begin();
    applyToHall(m_store.settings());
}

const HardwareSettings& HardwareSettingsController::settings() const
{
    return m_store.settings();
}

bool HardwareSettingsController::loadedFromEeprom() const
{
    return m_store.loadedFromEeprom();
}

bool HardwareSettingsController::usedFactoryFallback() const
{
    return m_store.usedFactoryFallback();
}

bool HardwareSettingsController::safeToChange() const
{
    switch (m_machine.state())
    {
        case MachineState::Winding:
        case MachineState::Paused:
        case MachineState::ManualRun:
        case MachineState::CoilComplete:
        case MachineState::Fault:
            return false;

        case MachineState::EnterCoilCount:
        case MachineState::EnterTurns:
        case MachineState::Ready:
        case MachineState::JobComplete:
            return true;
    }

    return false;
}

HardwareSettingsApplyResult HardwareSettingsController::apply(
    const HardwareSettings& settings)
{
    if (!settings.isValid()) return HardwareSettingsApplyResult::Invalid;
    if (!safeToChange()) return HardwareSettingsApplyResult::Busy;
    if (!m_store.save(settings))
        return HardwareSettingsApplyResult::PersistenceFailed;

    applyToHall(m_store.settings());
    return HardwareSettingsApplyResult::Applied;
}

HardwareSettingsApplyResult
HardwareSettingsController::resetToFactoryDefaults()
{
    if (!safeToChange()) return HardwareSettingsApplyResult::Busy;
    if (!m_store.resetToFactoryDefaults())
        return HardwareSettingsApplyResult::PersistenceFailed;

    applyToHall(m_store.settings());
    return HardwareSettingsApplyResult::Applied;
}

void HardwareSettingsController::applyToHall(
    const HardwareSettings& settings)
{
    m_hall.setThreshold(settings.hallThreshold);
    m_hall.setHysteresis(settings.hallHysteresis);
    m_hall.setReleaseDebounceMs(settings.hallReleaseDebounceMs);
}

} // namespace CM
