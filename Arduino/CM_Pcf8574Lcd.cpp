#include "CM_Pcf8574Lcd.h"

#if !defined(__AVR__)
#error "CM_Pcf8574Lcd is intentionally limited to the AVR Arduino runtime"
#endif

#include <avr/io.h>
#include <avr/pgmspace.h>

namespace CM
{
namespace
{
constexpr uint8_t TwiStatusMask = 0xF8U;
constexpr uint8_t TwiStart = 0x08U;
constexpr uint8_t TwiRepeatedStart = 0x10U;
constexpr uint8_t TwiSlaWriteAck = 0x18U;
constexpr uint8_t TwiDataAck = 0x28U;
constexpr uint32_t TwiFrequencyHz = 100000UL;
constexpr uint16_t TwiWaitLimit = 60000U;

uint8_t twiStatus()
{
    return static_cast<uint8_t>(TWSR & TwiStatusMask);
}
}

Pcf8574Lcd::Pcf8574Lcd(uint8_t address)
    : m_address(address),
      m_backlightMask(0U)
{
}

bool Pcf8574Lcd::waitForTwint()
{
    for (uint16_t count = 0U; count < TwiWaitLimit; ++count)
    {
        if ((TWCR & _BV(TWINT)) != 0U) return true;
    }
    return false;
}

bool Pcf8574Lcd::twiWrite(uint8_t value) const
{
    TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWSTA) | _BV(TWEN));
    if (!waitForTwint())
    {
        TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN) | _BV(TWSTO));
        return false;
    }

    const uint8_t startStatus = twiStatus();
    if (startStatus != TwiStart && startStatus != TwiRepeatedStart)
    {
        TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN) | _BV(TWSTO));
        return false;
    }

    TWDR = static_cast<uint8_t>(m_address << 1U);
    TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN));
    if (!waitForTwint() || twiStatus() != TwiSlaWriteAck)
    {
        TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN) | _BV(TWSTO));
        return false;
    }

    TWDR = value;
    TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN));
    const bool acknowledged = waitForTwint() && twiStatus() == TwiDataAck;
    TWCR = static_cast<uint8_t>(_BV(TWINT) | _BV(TWEN) | _BV(TWSTO));
    return acknowledged;
}

bool Pcf8574Lcd::expanderWrite(uint8_t value) const
{
    return twiWrite(static_cast<uint8_t>(value | m_backlightMask));
}

bool Pcf8574Lcd::pulseEnable(uint8_t value) const
{
    if (!expanderWrite(static_cast<uint8_t>(value | EnableMask))) return false;
    delayMicroseconds(1U);
    if (!expanderWrite(static_cast<uint8_t>(value & static_cast<uint8_t>(~EnableMask))))
        return false;
    delayMicroseconds(50U);
    return true;
}

bool Pcf8574Lcd::write4Bits(uint8_t value) const
{
    return expanderWrite(value) && pulseEnable(value);
}

bool Pcf8574Lcd::send(uint8_t value, uint8_t mode) const
{
    const uint8_t highNibble = static_cast<uint8_t>((value & 0xF0U) | mode);
    const uint8_t lowNibble = static_cast<uint8_t>(((value << 4U) & 0xF0U) | mode);
    return write4Bits(highNibble) && write4Bits(lowNibble);
}

bool Pcf8574Lcd::command(uint8_t value) const
{
    return send(value, 0U);
}

bool Pcf8574Lcd::write(uint8_t value) const
{
    return send(value, RegisterSelectMask);
}

bool Pcf8574Lcd::init()
{
    TWSR = static_cast<uint8_t>(TWSR & static_cast<uint8_t>(~0x03U));
    TWBR = static_cast<uint8_t>(((F_CPU / TwiFrequencyHz) - 16UL) / 2UL);
    TWCR = static_cast<uint8_t>(_BV(TWEN));

    delay(50U);
    if (!expanderWrite(0U)) return false;

    if (!write4Bits(0x30U)) return false;
    delayMicroseconds(4500U);
    if (!write4Bits(0x30U)) return false;
    delayMicroseconds(4500U);
    if (!write4Bits(0x30U)) return false;
    delayMicroseconds(150U);
    if (!write4Bits(0x20U)) return false;

    if (!command(0x28U)) return false;
    if (!command(0x08U)) return false;
    if (!command(0x01U)) return false;
    delayMicroseconds(2000U);
    if (!command(0x06U)) return false;
    return command(0x0CU);
}

void Pcf8574Lcd::backlight()
{
    m_backlightMask = BacklightMask;
    (void)expanderWrite(0U);
}

void Pcf8574Lcd::clear()
{
    (void)command(0x01U);
    delayMicroseconds(2000U);
}

void Pcf8574Lcd::setCursor(uint8_t column, uint8_t row)
{
    static const uint8_t RowOffsets[2] = {0x00U, 0x40U};
    const uint8_t safeRow = row > 1U ? 1U : row;
    (void)command(static_cast<uint8_t>(0x80U | (column + RowOffsets[safeRow])));
}

void Pcf8574Lcd::createChar(uint8_t slot, const uint8_t rows[8])
{
    if (rows == nullptr) return;
    const uint8_t safeSlot = static_cast<uint8_t>(slot & 0x07U);
    if (!command(static_cast<uint8_t>(0x40U | (safeSlot << 3U)))) return;
    for (uint8_t row = 0U; row < 8U; ++row)
        (void)write(static_cast<uint8_t>(rows[row] & 0x1FU));
}

void Pcf8574Lcd::print(const char* text)
{
    if (text == nullptr) return;
    while (*text != '\0')
        (void)write(static_cast<uint8_t>(*text++));
}

void Pcf8574Lcd::print(const __FlashStringHelper* text)
{
    if (text == nullptr) return;
    PGM_P cursor = reinterpret_cast<PGM_P>(text);
    for (;;)
    {
        const char value = static_cast<char>(pgm_read_byte(cursor++));
        if (value == '\0') return;
        (void)write(static_cast<uint8_t>(value));
    }
}

void Pcf8574Lcd::print(char value)
{
    (void)write(static_cast<uint8_t>(value));
}

void Pcf8574Lcd::print(uint8_t value)
{
    char digits[4];
    utoa(value, digits, 10);
    print(digits);
}

void Pcf8574Lcd::print(uint8_t value, int base)
{
    char digits[4];
    const uint8_t radix = base == HEX ? 16U : 10U;
    utoa(value, digits, radix);
    print(digits);
}

} // namespace CM
