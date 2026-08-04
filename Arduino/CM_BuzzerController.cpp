#include "CM_BuzzerController.h"

namespace CM
{
namespace
{
constexpr uint16_t OnDurationMs = 150U;
constexpr uint16_t OffDurationMs = 100U;
constexpr uint8_t FinalStep = 5U;
}

BuzzerController::BuzzerController(uint8_t pin, bool activeHigh)
    : m_pin(pin),
      m_activeHigh(activeHigh),
      m_outputOn(false),
      m_active(false),
      m_finished(false),
      m_step(0U),
      m_stepStartedMs(0UL)
{
}

void BuzzerController::begin()
{
    pinMode(m_pin, OUTPUT);
    stop();
}

void BuzzerController::startCompletionPattern(uint32_t nowMs)
{
    m_active = true;
    m_finished = false;
    m_step = 0U;
    m_stepStartedMs = nowMs;
    writeOutput(true);
}

void BuzzerController::update(uint32_t nowMs)
{
    if (!m_active)
    {
        return;
    }

    const uint16_t duration = m_outputOn ? OnDurationMs : OffDurationMs;
    if (static_cast<uint32_t>(nowMs - m_stepStartedMs) < duration)
    {
        return;
    }

    ++m_step;
    m_stepStartedMs = nowMs;

    if (m_step > FinalStep)
    {
        writeOutput(false);
        m_active = false;
        m_finished = true;
        return;
    }

    writeOutput(!m_outputOn);
}

void BuzzerController::stop()
{
    writeOutput(false);
    m_active = false;
    m_finished = false;
    m_step = 0U;
}

bool BuzzerController::isActive() const
{
    return m_active;
}

bool BuzzerController::patternFinished() const
{
    return m_finished;
}

void BuzzerController::writeOutput(bool enabled)
{
    m_outputOn = enabled;
    digitalWrite(m_pin,
                 (enabled == m_activeHigh) ? HIGH : LOW);
}

} // namespace CM
