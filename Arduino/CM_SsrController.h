#ifndef CM_SSR_CONTROLLER_H
#define CM_SSR_CONTROLLER_H

#include <Arduino.h>

#include "../Core/CM_StateMachine.h"

namespace CM
{

/**
 * @brief Safe adapter for the SSR-40DA control input.
 *
 * The SSR is enabled for Winding/ManualRun or for an explicit Arduino-local
 * service permit. Simulation mode always forces the physical output off.
 * ESP32/Web never receive direct access to this controller.
 */
class SsrController
{
public:
    explicit SsrController(uint8_t outputPin, bool activeHigh = true);

    void begin();
    void update(MachineState state,
                bool simulationMode,
                bool serviceMotorPermit = false);
    void forceOff();

    bool isCommandedOn() const;
    bool physicalOutputOn() const;

private:
    void writePhysical(bool enabled);

    uint8_t m_outputPin;
    bool m_activeHigh;
    bool m_commandedOn;
    bool m_physicalOutputOn;
};

} // namespace CM

#endif // CM_SSR_CONTROLLER_H
