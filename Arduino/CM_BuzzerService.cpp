#include "CM_BuzzerService.h"

namespace CM
{
namespace
{
constexpr uint16_t OnDurationMs = 150U;
constexpr uint16_t OffDurationMs = 100U;
constexpr uint8_t PhaseCount = 6U; // ON/OFF repeated three times
}

BuzzerService::BuzzerService(uint8_t pin, bool activeHigh)
    : m_pin(pin),
      m_activeHigh(activeHigh),
      m_active(false),
      m_outputOn(false),
      m_finishedEvent(false),
      m_phase(0U),
      m_phaseStartedMs(0UL)
{
}

void BuzzerService::begin()
{
    pinMode(m_pin, OUTPUT);
    stop();
}

void BuzzerService::startCompletionSignal(uint32_t nowMs)
{
    m_active = true;
    m_finishedEvent = false;
    m_phase = 0U;
    m_phaseStartedMs = nowMs;
    writeOutput(true);
}

void BuzzerService::update(uint32_t nowMs)
{
    if (!m_active)
    {
        return;
    }

    const uint16_t duration = m_outputOn ? OnDurationMs : OffDurationMs;
    if (static_cast<uint32_t>(nowMs - m_phaseStartedMs) < duration)
    {
        return;
    }

    ++m_phase;
    m_phaseStartedMs = nowMs;

    if (m_phase >= PhaseCount)
    {
        writeOutput(false);
        m_active = false;
        m_finishedEvent = true;
        return;
    }

    writeOutput(!m_outputOn);
}

void BuzzerService::stop()
{
    writeOutput(false);
    m_active = false;
    m_finishedEvent = false;
    m_phase = 0U;
}

bool BuzzerService::isActive() const
{
    return m_active;
}

bool BuzzerService::takeFinishedEvent()
{
    const bool result = m_finishedEvent;
    m_finishedEvent = false;
    return result;
}

void BuzzerService::writeOutput(bool enabled)
{
    m_outputOn = enabled;
    digitalWrite(m_pin,
                 (enabled == m_activeHigh) ? HIGH : LOW);
}
}
