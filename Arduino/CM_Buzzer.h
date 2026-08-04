#ifndef CM_BUZZER_H
#define CM_BUZZER_H

#include <Arduino.h>

namespace CM
{

class Buzzer
{
public:
    explicit Buzzer(uint8_t pin, bool activeHigh = true);

    void begin();
    void startCompletionPattern(uint32_t nowMs);
    void update(uint32_t nowMs);
    void stop();

    bool isActive() const;
    bool consumeFinishedEvent();

private:
    void writeOutput(bool enabled);

    uint8_t m_pin;
    bool m_activeHigh;
    bool m_active;
    bool m_outputOn;
    bool m_finishedEvent;
    uint8_t m_step;
    uint32_t m_stepStartedMs;
};

} // namespace CM

#endif // CM_BUZZER_H
