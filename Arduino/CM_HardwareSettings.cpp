#include "CM_HardwareSettings.h"

#include <EEPROM.h>
#include <stddef.h>
#include <string.h>

namespace CM
{

HardwareSettings::HardwareSettings()
    : hallThreshold(590U),
      hallHysteresis(50U),
      hallReleaseDebounceMs(25U),
      hallDirection(HallSignalDirection::Rising)
{
}

bool HardwareSettings::isValid() const
{
    if (hallThreshold == 0U || hallThreshold > 1023U) return false;
    if (hallHysteresis == 0U || hallHysteresis > 512U) return false;
    if (hallHysteresis >= hallThreshold) return false;
    if (hallReleaseDebounceMs == 0U || hallReleaseDebounceMs > 1000U)
        return false;
    return hallDirection == HallSignalDirection::Rising ||
           hallDirection == HallSignalDirection::Falling;
}

bool HardwareSettings::equals(const HardwareSettings& other) const
{
    return hallThreshold == other.hallThreshold &&
           hallHysteresis == other.hallHysteresis &&
           hallReleaseDebounceMs == other.hallReleaseDebounceMs &&
           hallDirection == other.hallDirection;
}

HardwareSettingsStore::HardwareSettingsStore()
    : m_settings(factoryDefaults()),
      m_sequence(0UL),
      m_activeSlot(-1),
      m_loadedFromEeprom(false),
      m_factoryFallback(false)
{
}

void HardwareSettingsStore::begin()
{
    StoredSlot slots[SlotCount];
    bool valid[SlotCount] = {false, false};

    for (uint8_t index = 0U; index < SlotCount; ++index)
    {
        valid[index] = loadSlot(index, slots[index]) && slotValid(slots[index]);
    }

    int8_t selected = -1;
    if (valid[0U] && valid[1U])
    {
        selected = sequenceNewer(slots[1U].sequence, slots[0U].sequence)
                       ? 1
                       : 0;
    }
    else if (valid[0U])
    {
        selected = 0;
    }
    else if (valid[1U])
    {
        selected = 1;
    }

    HardwareSettings decoded;
    if (selected >= 0 && decode(slots[static_cast<uint8_t>(selected)], decoded))
    {
        m_settings = decoded;
        m_sequence = slots[static_cast<uint8_t>(selected)].sequence;
        m_activeSlot = selected;
        m_loadedFromEeprom = true;
        m_factoryFallback = false;
        return;
    }

    m_settings = factoryDefaults();
    m_sequence = 0UL;
    m_activeSlot = -1;
    m_loadedFromEeprom = false;
    m_factoryFallback = true;
}

const HardwareSettings& HardwareSettingsStore::settings() const
{
    return m_settings;
}

bool HardwareSettingsStore::save(const HardwareSettings& settings)
{
    if (!settings.isValid()) return false;
    if (m_loadedFromEeprom && settings.equals(m_settings)) return true;

    const uint8_t targetSlot = m_activeSlot == 0 ? 1U : 0U;
    uint32_t nextSequence = m_sequence + 1UL;
    if (nextSequence == 0UL) nextSequence = 1UL;

    StoredSlot encoded;
    encode(settings, nextSequence, encoded);
    const int address = slotAddress(targetSlot);
    if (address < 0) return false;

    EEPROM.put(address, encoded);

    StoredSlot verify;
    if (!loadSlot(targetSlot, verify) || !slotValid(verify)) return false;
    HardwareSettings decoded;
    if (!decode(verify, decoded) || !decoded.equals(settings)) return false;

    m_settings = decoded;
    m_sequence = verify.sequence;
    m_activeSlot = static_cast<int8_t>(targetSlot);
    m_loadedFromEeprom = true;
    m_factoryFallback = false;
    return true;
}

bool HardwareSettingsStore::resetToFactoryDefaults()
{
    return save(factoryDefaults());
}

bool HardwareSettingsStore::loadedFromEeprom() const
{
    return m_loadedFromEeprom;
}

bool HardwareSettingsStore::usedFactoryFallback() const
{
    return m_factoryFallback;
}

HardwareSettings HardwareSettingsStore::factoryDefaults()
{
    return HardwareSettings();
}

uint16_t HardwareSettingsStore::calculateCrc(const uint8_t* data,
                                             size_t length)
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

bool HardwareSettingsStore::sequenceNewer(uint32_t candidate,
                                          uint32_t reference)
{
    return static_cast<int32_t>(candidate - reference) > 0;
}

int HardwareSettingsStore::slotAddress(uint8_t slotIndex) const
{
    if (slotIndex >= SlotCount) return -1;

    const size_t reservedBytes = sizeof(StoredSlot) * SlotCount;
    const size_t eepromLength = static_cast<size_t>(EEPROM.length());
    if (reservedBytes > eepromLength) return -1;

    const size_t base = eepromLength - reservedBytes;
    const size_t address = base + sizeof(StoredSlot) * slotIndex;
    if (address + sizeof(StoredSlot) > eepromLength) return -1;
    return static_cast<int>(address);
}

bool HardwareSettingsStore::loadSlot(uint8_t slotIndex, StoredSlot& slot) const
{
    const int address = slotAddress(slotIndex);
    if (address < 0) return false;
    EEPROM.get(address, slot);
    return true;
}

bool HardwareSettingsStore::slotValid(const StoredSlot& slot) const
{
    if (slot.magic != Magic || slot.version != Version || slot.sequence == 0UL)
        return false;
    if (slot.hallDirection > static_cast<uint8_t>(HallSignalDirection::Falling))
        return false;

    const uint16_t expected = calculateCrc(
        reinterpret_cast<const uint8_t*>(&slot),
        offsetof(StoredSlot, crc));
    if (expected != slot.crc) return false;

    HardwareSettings decoded;
    return decode(slot, decoded);
}

bool HardwareSettingsStore::decode(const StoredSlot& slot,
                                   HardwareSettings& settings) const
{
    settings.hallThreshold = slot.hallThreshold;
    settings.hallHysteresis = slot.hallHysteresis;
    settings.hallReleaseDebounceMs = slot.hallReleaseDebounceMs;
    settings.hallDirection =
        slot.hallDirection == static_cast<uint8_t>(HallSignalDirection::Falling)
            ? HallSignalDirection::Falling
            : HallSignalDirection::Rising;
    return settings.isValid();
}

void HardwareSettingsStore::encode(const HardwareSettings& settings,
                                   uint32_t sequence,
                                   StoredSlot& slot) const
{
    memset(&slot, 0, sizeof(slot));
    slot.magic = Magic;
    slot.version = Version;
    slot.hallDirection = static_cast<uint8_t>(settings.hallDirection);
    slot.sequence = sequence;
    slot.hallThreshold = settings.hallThreshold;
    slot.hallHysteresis = settings.hallHysteresis;
    slot.hallReleaseDebounceMs = settings.hallReleaseDebounceMs;
    slot.crc = calculateCrc(
        reinterpret_cast<const uint8_t*>(&slot),
        offsetof(StoredSlot, crc));
}

} // namespace CM
