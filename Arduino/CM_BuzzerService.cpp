#include "CM_BuzzerService.h"

namespace CM
{
namespace
{
constexpr uint16_t CoilSignalOnMs = 400U;
constexpr uint16_t ProgramSignalOnMs = 150U;
constexpr uint16_t ProgramSignalOffMs = 100U;
}

BuzzerService::BuzzerService(uint8_t pin, bool activeHigh)
    : m_pin(pin),
      m_activeHigh(activeHigh),
      m_active(false),
      m_outputOn(false),
      m_finishedEvent(false),
      m_phase(0U),
      m_phaseCount(0U),
      m_onDurationMs(0U),
      m_offDurationMs(0U),
      m_phaseStartedMs(0UL)
{
}

void BuzzerService::begin()
{
    pinMode(m_pin, OUTPUT);
    stop();
}

void BuzzerService::startCoilCompleteSignal(uint32_t nowMs)
{
    startPattern(nowMs, 1U, CoilSignalOnMs, 0U);
}

void BuzzerService::startProgramCompleteSignal(uint32_t nowMs)
{
    startPattern(nowMs,
                 3U,
                 ProgramSignalOnMs,
                 ProgramSignalOffMs);
}

void BuzzerService::startPattern(uint32_t nowMs,
                                 uint8_t signalCount,
                                 uint16_t onDurationMs,
                                 uint16_t offDurationMs)
{
    if (signalCount == 0U || onDurationMs == 0U)
    {
        stop();
        return;
    }

    m_active = true;
    m_finishedEvent = false;
    m_phase = 0U;
    m_phaseCount = static_cast<uint8_t>(signalCount * 2U - 1U);
    m_onDurationMs = onDurationMs;
    m_offDurationMs = offDurationMs;
    m_phaseStartedMs = nowMs;
    writeOutput(true);
}

void BuzzerService::update(uint32_t nowMs)
{
    if (!m_active)
    {
        return;
    }

    const uint16_t duration = m_outputOn ? m_onDurationMs : m_offDurationMs;
    if (static_cast<uint32_t>(nowMs - m_phaseStartedMs) < duration)
    {
        return;
    }

    ++m_phase;
    m_phaseStartedMs = nowMs;

    if (m_phase >= m_phaseCount)
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
    m_phaseCount = 0U;
    m_onDurationMs = 0U;
    m_offDurationMs = 0U;
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
