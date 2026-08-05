#include "CM_EepromPersistence.h"

#include <EEPROM.h>
#include <stddef.h>
#include <string.h>

namespace CM
{
EepromPersistence::EepromPersistence()
    : m_state()
{
    resetDefaults();
}

void EepromPersistence::begin()
{
    EEPROM.get(EepromAddress, m_state);
    if (!isValid())
    {
        resetDefaults();
        persist();
    }
}

uint32_t EepromPersistence::nextSessionId() const
{
    return m_state.nextSessionId;
}

uint32_t EepromPersistence::nextRunId() const
{
    return m_state.nextRunId;
}

void EepromPersistence::saveNextIdentifiers(uint32_t nextSessionId,
                                            uint32_t nextRunId)
{
    if (nextSessionId == 0UL)
    {
        nextSessionId = 1UL;
    }
    if (nextRunId == 0UL)
    {
        nextRunId = 1UL;
    }

    if (m_state.nextSessionId == nextSessionId &&
        m_state.nextRunId == nextRunId)
    {
        return;
    }

    m_state.nextSessionId = nextSessionId;
    m_state.nextRunId = nextRunId;
    persist();
}

bool EepromPersistence::addPendingCompleted(const WindingEvent& event)
{
    if (event.type != WindingEventType::RunCompleted ||
        event.sessionId == 0UL || event.runId == 0UL)
    {
        return false;
    }

    for (uint8_t index = 0U; index < m_state.count; ++index)
    {
        if (m_state.pending[index].runId == event.runId)
        {
            return true;
        }
    }

    if (m_state.count >= PendingCapacity)
    {
        return false;
    }

    StoredEvent& target = m_state.pending[m_state.count];
    target.sessionId = event.sessionId;
    target.runId = event.runId;
    target.completedRuns = event.completedRuns;
    ++m_state.count;
    persist();
    return true;
}

bool EepromPersistence::removePendingCompleted(uint32_t runId)
{
    for (uint8_t index = 0U; index < m_state.count; ++index)
    {
        if (m_state.pending[index].runId != runId)
        {
            continue;
        }

        for (uint8_t move = index; move + 1U < m_state.count; ++move)
        {
            m_state.pending[move] = m_state.pending[move + 1U];
        }

        --m_state.count;
        memset(&m_state.pending[m_state.count], 0, sizeof(StoredEvent));
        persist();
        return true;
    }

    return false;
}

uint8_t EepromPersistence::pendingCount() const
{
    return m_state.count;
}

bool EepromPersistence::pendingAt(uint8_t index, WindingEvent& event) const
{
    if (index >= m_state.count)
    {
        return false;
    }

    event.type = WindingEventType::RunCompleted;
    event.sessionId = m_state.pending[index].sessionId;
    event.runId = m_state.pending[index].runId;
    event.completedRuns = m_state.pending[index].completedRuns;
    return true;
}

void EepromPersistence::resetDefaults()
{
    memset(&m_state, 0, sizeof(m_state));
    m_state.magic = Magic;
    m_state.version = Version;
    m_state.count = 0U;
    m_state.nextSessionId = 1UL;
    m_state.nextRunId = 1UL;
    m_state.crc = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_state),
        offsetof(StoredState, crc));
}

void EepromPersistence::persist()
{
    m_state.crc = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_state),
        offsetof(StoredState, crc));
    EEPROM.put(EepromAddress, m_state);
}

bool EepromPersistence::isValid() const
{
    if (m_state.magic != Magic ||
        m_state.version != Version ||
        m_state.count > PendingCapacity ||
        m_state.nextSessionId == 0UL ||
        m_state.nextRunId == 0UL)
    {
        return false;
    }

    const uint16_t expected = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_state),
        offsetof(StoredState, crc));
    return expected == m_state.crc;
}

uint16_t EepromPersistence::calculateCrc(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 1U) != 0U
                      ? static_cast<uint16_t>((crc >> 1U) ^ 0xA001U)
                      : static_cast<uint16_t>(crc >> 1U);
        }
    }
    return crc;
}
}
