#include "CM_BuzzerService.h"

namespace CM
{
namespace
{
constexpr uint16_t JobAcceptedSignalMs = 80U;
constexpr uint16_t CoilSignalMs = 120U;
constexpr uint16_t ProgramSignalMs = 120U;
}

BuzzerService::BuzzerService(uint8_t pin, bool activeHigh)
    : m_pin(pin),
      m_activeHigh(activeHigh),
      m_active(false),
      m_outputOn(false),
      m_finishedEvent(false),
      m_phase(0U),
      m_phaseCount(0U),
      m_phaseDurationMs(0U),
      m_phaseStartedMs(0UL)
{
}

void BuzzerService::begin()
{
    pinMode(m_pin, OUTPUT);
    stop();
}

void BuzzerService::startJobAcceptedSignal(uint32_t nowMs)
{
    startPattern(nowMs, 2U, JobAcceptedSignalMs);
}

void BuzzerService::startCoilCompleteSignal(uint32_t nowMs)
{
    startPattern(nowMs, 1U, CoilSignalMs);
}

void BuzzerService::startProgramCompleteSignal(uint32_t nowMs)
{
    startPattern(nowMs, 3U, ProgramSignalMs);
}

void BuzzerService::startPattern(uint32_t nowMs,
                                 uint8_t signalCount,
                                 uint16_t phaseDurationMs)
{
    if (signalCount == 0U || phaseDurationMs == 0U)
    {
        stop();
        return;
    }

    m_active = true;
    m_finishedEvent = false;
    m_phase = 0U;
    m_phaseCount = static_cast<uint8_t>(signalCount * 2U - 1U);
    m_phaseDurationMs = phaseDurationMs;
    m_phaseStartedMs = nowMs;
    writeOutput(true);
}

void BuzzerService::update(uint32_t nowMs)
{
    if (!m_active)
    {
        return;
    }

    if (static_cast<uint32_t>(nowMs - m_phaseStartedMs) < m_phaseDurationMs)
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
    m_phaseDurationMs = 0U;
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
