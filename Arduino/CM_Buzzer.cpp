#include "CM_Buzzer.h"

namespace CM
{

namespace
{
constexpr uint16_t OnMs = 150U;
constexpr uint16_t OffMs = 100U;
constexpr uint8_t FinalStep = 6U;
}

Buzzer::Buzzer(uint8_t pin, bool activeHigh)
    : m_pin(pin),
      m_activeHigh(activeHigh),
      m_active(false),
      m_outputOn(false),
      m_finishedEvent(false),
      m_step(0U),
      m_stepStartedMs(0UL)
{
}

void Buzzer::begin()
{
    pinMode(m_pin, OUTPUT);
    writeOutput(false);
}

void Buzzer::startCompletionPattern(uint32_t nowMs)
{
    m_active = true;
    m_finishedEvent = false;
    m_step = 0U;
    m_stepStartedMs = nowMs;
    writeOutput(true);
}

void Buzzer::update(uint32_t nowMs)
{
    if (!m_active)
    {
        return;
    }

    const uint16_t duration = m_outputOn ? OnMs : OffMs;
    if ((nowMs - m_stepStartedMs) < duration)
    {
        return;
    }

    ++m_step;
    m_stepStartedMs = nowMs;

    if (m_step >= FinalStep)
    {
        stop();
        m_finishedEvent = true;
        return;
    }

    writeOutput(!m_outputOn);
}

void Buzzer::stop()
{
    m_active = false;
    writeOutput(false);
}

bool Buzzer::isActive() const
{
    return m_active;
}

bool Buzzer::consumeFinishedEvent()
{
    const bool result = m_finishedEvent;
    m_finishedEvent = false;
    return result;
}

void Buzzer::writeOutput(bool enabled)
{
    m_outputOn = enabled;
    digitalWrite(m_pin, (enabled == m_activeHigh) ? HIGH : LOW);
}

} // namespace CM
