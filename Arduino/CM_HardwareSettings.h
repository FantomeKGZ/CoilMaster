#ifndef CM_HARDWARE_SETTINGS_H
#define CM_HARDWARE_SETTINGS_H

#include <Arduino.h>

namespace CM
{

enum class HallSignalDirection : uint8_t
{
    Rising = 0U,
    Falling = 1U
};

struct HardwareSettings
{
    uint16_t hallThreshold;
    uint16_t hallHysteresis;
    uint16_t hallReleaseDebounceMs;
    HallSignalDirection hallDirection;

    HardwareSettings();

    bool isValid() const;
    bool equals(const HardwareSettings& other) const;
};

class HardwareSettingsStore
{
public:
    HardwareSettingsStore();

    void begin();

    const HardwareSettings& settings() const;
    bool save(const HardwareSettings& settings);
    bool resetToFactoryDefaults();

    bool loadedFromEeprom() const;
    bool usedFactoryFallback() const;

private:
    static constexpr uint16_t Magic = 0x4853U;
    static constexpr uint8_t Version = 1U;
    static constexpr uint8_t SlotCount = 2U;

    struct StoredSlot
    {
        uint16_t magic;
        uint8_t version;
        uint8_t hallDirection;
        uint32_t sequence;
        uint16_t hallThreshold;
        uint16_t hallHysteresis;
        uint16_t hallReleaseDebounceMs;
        uint16_t reserved;
        uint16_t crc;
    };

    static HardwareSettings factoryDefaults();
    static uint16_t calculateCrc(const uint8_t* data, size_t length);
    static bool sequenceNewer(uint32_t candidate, uint32_t reference);

    int slotAddress(uint8_t slotIndex) const;
    bool loadSlot(uint8_t slotIndex, StoredSlot& slot) const;
    bool slotValid(const StoredSlot& slot) const;
    bool decode(const StoredSlot& slot, HardwareSettings& settings) const;
    void encode(const HardwareSettings& settings,
                uint32_t sequence,
                StoredSlot& slot) const;

    HardwareSettings m_settings;
    uint32_t m_sequence;
    int8_t m_activeSlot;
    bool m_loadedFromEeprom;
    bool m_factoryFallback;
};

} // namespace CM

#endif // CM_HARDWARE_SETTINGS_H
