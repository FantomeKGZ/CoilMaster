#ifndef CM_PCF8574_LCD_H
#define CM_PCF8574_LCD_H

#include <Arduino.h>

namespace CM
{

class Pcf8574Lcd
{
public:
    explicit Pcf8574Lcd(uint8_t address);

    bool init();
    void backlight();
    void clear();
    void setCursor(uint8_t column, uint8_t row);
    void print(const char* text);

private:
    static constexpr uint8_t EnableMask = 0x04U;
    static constexpr uint8_t RegisterSelectMask = 0x01U;
    static constexpr uint8_t BacklightMask = 0x08U;

    bool twiWrite(uint8_t value) const;
    static bool waitForTwint();
    bool expanderWrite(uint8_t value) const;
    bool pulseEnable(uint8_t value) const;
    bool write4Bits(uint8_t value) const;
    bool send(uint8_t value, uint8_t mode) const;
    bool command(uint8_t value) const;
    bool write(uint8_t value) const;

    uint8_t m_address;
    uint8_t m_backlightMask;
};

} // namespace CM

#endif // CM_PCF8574_LCD_H
