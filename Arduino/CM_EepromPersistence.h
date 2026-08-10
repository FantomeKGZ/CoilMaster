#ifndef CM_EEPROM_PERSISTENCE_H
#define CM_EEPROM_PERSISTENCE_H

#include <Arduino.h>

#include "../Core/CM_WindingEvent.h"
#include "../Core/CM_WindingJob.h"

namespace CM
{
class EepromPersistence
{
public:
    static constexpr uint8_t PendingCapacity = 4U;

    EepromPersistence();

    void begin();

    uint32_t nextSessionId() const;
    uint32_t nextRunId() const;
    void saveNextIdentifiers(uint32_t nextSessionId, uint32_t nextRunId);

    bool addPendingCompleted(const WindingEvent& event);
    bool addPendingCompleted(const WindingEvent& event,
                             const WindingJob& job);
    bool removePendingCompleted(uint32_t runId);
    uint8_t pendingCount() const;
    bool pendingAt(uint8_t index, WindingEvent& event) const;
    bool pendingAt(uint8_t index,
                   WindingEvent& event,
                   WindingJob& job,
                   bool& hasJobMetadata) const;

private:
    static constexpr uint16_t Magic = 0x434DU;
    static constexpr uint8_t Version = 1U;
    static constexpr uint16_t MetadataMagic = 0x4A4DU;
    static constexpr uint8_t MetadataVersion = 1U;
    static constexpr int EepromAddress = 0;

    struct StoredEvent
    {
        uint32_t sessionId;
        uint32_t runId;
        uint16_t completedRuns;
    };

    struct StoredState
    {
        uint16_t magic;
        uint8_t version;
        uint8_t count;
        uint32_t nextSessionId;
        uint32_t nextRunId;
        StoredEvent pending[PendingCapacity];
        uint16_t crc;
    };

    struct StoredJobMetadata
    {
        uint32_t runId;
        uint8_t valid;
        uint8_t source;
        uint8_t windingType;
        uint8_t coilCount;
        uint16_t targetTurns[MaxCoilsPerJob];
    };

    struct StoredMetadataState
    {
        uint16_t magic;
        uint8_t version;
        uint8_t count;
        StoredJobMetadata pending[PendingCapacity];
        uint16_t crc;
    };

    void resetDefaults();
    void persist();
    bool isValid() const;

    int metadataAddress() const;
    void resetMetadata();
    void persistMetadata();
    bool metadataValid() const;
    bool storeMetadata(uint8_t index, const WindingJob& job);

    static uint16_t calculateCrc(const uint8_t* data, size_t length);

    StoredState m_state;
    StoredMetadataState m_metadata;
};
}

#endif // CM_EEPROM_PERSISTENCE_H
