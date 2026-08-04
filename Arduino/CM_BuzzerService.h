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
    void startCompletionSignal(uint32_t nowMs);
    void update(uint32_t nowMs);
    void stop();

    bool isActive() const;
    bool takeFinishedEvent();

private:
    void writeOutput(bool enabled);

    uint8_t m_pin;
    bool m_activeHigh;
    bool m_active;
    bool m_outputOn;
    bool m_finishedEvent;
    uint8_t m_phase;
    uint32_t m_phaseStartedMs;
};
}

#endif // CM_BUZZER_SERVICE_H
