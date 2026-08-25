#ifndef CM_BUZZER_SERVICE_H
#define CM_BUZZER_SERVICE_H

#include <Arduino.h>

namespace CM
{
class BuzzerService
{
public:
    explicit BuzzerService(uint8_t pin, bool activeHigh = true);

    void begin();

    /** Two short signals when a remote winding job is accepted. */
    void startJobAcceptedSignal(uint32_t nowMs);

    /** One short signal after an individual coil is completed. */
    void startCoilCompleteSignal(uint32_t nowMs);

    /** Three short signals after the whole winding program is completed. */
    void startProgramCompleteSignal(uint32_t nowMs);

    void update(uint32_t nowMs);
    void stop();

    bool isActive() const;
    bool takeFinishedEvent();

private:
    void startPattern(uint32_t nowMs,
                      uint8_t signalCount,
                      uint16_t phaseDurationMs);
    void writeOutput(bool enabled);

    uint8_t m_pin;
    bool m_activeHigh;
    bool m_active;
    bool m_outputOn;
    bool m_finishedEvent;
    uint8_t m_phase;
    uint8_t m_phaseCount;
    uint16_t m_phaseDurationMs;
    uint32_t m_phaseStartedMs;
};
}

#endif // CM_BUZZER_SERVICE_H
