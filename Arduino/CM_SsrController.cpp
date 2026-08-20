#include "CM_SsrController.h"

namespace CM
{

SsrController::SsrController(uint8_t outputPin, bool activeHigh)
    : m_outputPin(outputPin),
      m_activeHigh(activeHigh),
      m_commandedOn(false),
      m_physicalOutputOn(false)
{
}

void SsrController::begin()
{
    pinMode(m_outputPin, OUTPUT);
    forceOff();
}

void SsrController::update(MachineState state,
                           bool simulationMode,
                           bool serviceMotorPermit)
{
    m_commandedOn = state == MachineState::Winding ||
                    state == MachineState::ManualRun ||
                    serviceMotorPermit;

    writePhysical(m_commandedOn && !simulationMode);
}

void SsrController::forceOff()
{
    m_commandedOn = false;
    writePhysical(false);
}

bool SsrController::isCommandedOn() const
{
    return m_commandedOn;
}

bool SsrController::physicalOutputOn() const
{
    return m_physicalOutputOn;
}

void SsrController::writePhysical(bool enabled)
{
    m_physicalOutputOn = enabled;
    const uint8_t level = (enabled == m_activeHigh) ? HIGH : LOW;
    digitalWrite(m_outputPin, level);
}

} // namespace CM
