#include "CM_EepromPersistence.h"

#include <EEPROM.h>
#include <stddef.h>
#include <string.h>

namespace CM
{
EepromPersistence::EepromPersistence()
    : m_state(), m_metadata()
{
    resetDefaults();
    resetMetadata();
}

void EepromPersistence::begin()
{
    EEPROM.get(EepromAddress, m_state);
    if (!isValid())
    {
        resetDefaults();
        persist();
    }

    EEPROM.get(metadataAddress(), m_metadata);
    if (!metadataValid())
    {
        // Metadata is an additive v1 sidecar. Rebuilding it must never reset the
        // already-valid nextSessionId/nextRunId stored in the original state.
        resetMetadata();
        persistMetadata();
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

    if (!metadataValid())
    {
        resetMetadata();
    }

    const uint8_t index = m_state.count;
    StoredEvent& target = m_state.pending[index];
    target.sessionId = event.sessionId;
    target.runId = event.runId;
    target.completedRuns = event.completedRuns;
    ++m_state.count;

    m_metadata.count = m_state.count;
    memset(&m_metadata.pending[index], 0, sizeof(StoredJobMetadata));
    m_metadata.pending[index].runId = event.runId;

    persist();
    persistMetadata();
    return true;
}

bool EepromPersistence::addPendingCompleted(const WindingEvent& event,
                                            const WindingJob& job)
{
    if (!addPendingCompleted(event)) return false;

    for (uint8_t index = 0U; index < m_state.count; ++index)
    {
        if (m_state.pending[index].runId != event.runId) continue;
        if (!storeMetadata(index, job)) return false;
        persistMetadata();
        return true;
    }
    return false;
}

bool EepromPersistence::removePendingCompleted(uint32_t runId)
{
    for (uint8_t index = 0U; index < m_state.count; ++index)
    {
        if (m_state.pending[index].runId != runId)
        {
            continue;
        }

        if (!metadataValid()) resetMetadata();

        for (uint8_t move = index; move + 1U < m_state.count; ++move)
        {
            m_state.pending[move] = m_state.pending[move + 1U];
            m_metadata.pending[move] = m_metadata.pending[move + 1U];
        }

        --m_state.count;
        m_metadata.count = m_state.count;
        memset(&m_state.pending[m_state.count], 0, sizeof(StoredEvent));
        memset(&m_metadata.pending[m_metadata.count], 0, sizeof(StoredJobMetadata));
        persist();
        persistMetadata();
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

bool EepromPersistence::pendingAt(uint8_t index,
                                  WindingEvent& event,
                                  WindingJob& job,
                                  bool& hasJobMetadata) const
{
    hasJobMetadata = false;
    job.clear();
    if (!pendingAt(index, event)) return false;
    if (!metadataValid() || index >= m_metadata.count) return true;

    const StoredJobMetadata& source = m_metadata.pending[index];
    if (source.valid == 0U || source.runId != event.runId) return true;

    job.sessionId = event.sessionId;
    job.currentRunId = event.runId;
    job.completedRuns = event.completedRuns;
    job.source = source.source == static_cast<uint8_t>(JobSource::Esp32Web)
                     ? JobSource::Esp32Web
                     : JobSource::LocalKeypad;
    job.type = source.windingType == static_cast<uint8_t>(WindingType::Starting)
                   ? WindingType::Starting
                   : WindingType::Working;
    job.coilCount = source.coilCount;
    job.status = JobStatus::Completed;
    for (uint8_t coil = 0U; coil < job.coilCount; ++coil)
    {
        job.targetTurns[coil] = source.targetTurns[coil];
    }
    hasJobMetadata = job.isValid();
    if (!hasJobMetadata) job.clear();
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

int EepromPersistence::metadataAddress() const
{
    return EepromAddress + static_cast<int>(sizeof(StoredState));
}

void EepromPersistence::resetMetadata()
{
    memset(&m_metadata, 0, sizeof(m_metadata));
    m_metadata.magic = MetadataMagic;
    m_metadata.version = MetadataVersion;
    m_metadata.count = m_state.count;
    for (uint8_t index = 0U; index < m_state.count; ++index)
    {
        m_metadata.pending[index].runId = m_state.pending[index].runId;
        m_metadata.pending[index].valid = 0U;
    }
    m_metadata.crc = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_metadata),
        offsetof(StoredMetadataState, crc));
}

void EepromPersistence::persistMetadata()
{
    m_metadata.crc = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_metadata),
        offsetof(StoredMetadataState, crc));
    EEPROM.put(metadataAddress(), m_metadata);
}

bool EepromPersistence::metadataValid() const
{
    if (m_metadata.magic != MetadataMagic ||
        m_metadata.version != MetadataVersion ||
        m_metadata.count != m_state.count ||
        m_metadata.count > PendingCapacity)
    {
        return false;
    }

    const uint16_t expected = calculateCrc(
        reinterpret_cast<const uint8_t*>(&m_metadata),
        offsetof(StoredMetadataState, crc));
    if (expected != m_metadata.crc) return false;

    for (uint8_t index = 0U; index < m_metadata.count; ++index)
    {
        const StoredJobMetadata& item = m_metadata.pending[index];
        if (item.runId != m_state.pending[index].runId) return false;
        if (item.valid == 0U) continue;
        if (item.valid != 1U ||
            item.source > static_cast<uint8_t>(JobSource::Esp32Web) ||
            item.windingType > static_cast<uint8_t>(WindingType::Starting) ||
            item.coilCount == 0U || item.coilCount > MaxCoilsPerJob)
        {
            return false;
        }
        for (uint8_t coil = 0U; coil < item.coilCount; ++coil)
        {
            if (item.targetTurns[coil] == 0U ||
                item.targetTurns[coil] > MaxTurnsPerCoil)
            {
                return false;
            }
        }
    }
    return true;
}

bool EepromPersistence::storeMetadata(uint8_t index, const WindingJob& job)
{
    if (index >= m_state.count || !job.isValid()) return false;

    StoredJobMetadata& target = m_metadata.pending[index];
    memset(&target, 0, sizeof(target));
    target.runId = m_state.pending[index].runId;
    target.valid = 1U;
    target.source = static_cast<uint8_t>(job.source);
    target.windingType = static_cast<uint8_t>(job.type);
    target.coilCount = job.coilCount;
    for (uint8_t coil = 0U; coil < job.coilCount; ++coil)
    {
        target.targetTurns[coil] = job.targetTurns[coil];
    }
    return true;
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
