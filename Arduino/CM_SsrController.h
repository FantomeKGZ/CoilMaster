#ifndef CM_SSR_CONTROLLER_H
#define CM_SSR_CONTROLLER_H

#include <Arduino.h>

#include "../Core/CM_StateMachine.h"

namespace CM
{

/**
 * @brief Safe adapter for the SSR-40DA control input.
 *
 * The SSR is enabled only for Winding or ManualRun states. Simulation mode can
 * force the physical output off while the state machine continues to run.
 */
class SsrController
{
public:
    explicit SsrController(uint8_t outputPin, bool activeHigh = true);

    void begin();
    void update(MachineState state, bool simulationMode);
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
