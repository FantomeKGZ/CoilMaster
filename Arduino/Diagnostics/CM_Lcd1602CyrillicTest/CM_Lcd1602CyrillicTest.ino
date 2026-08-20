#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Standalone LCD1602 character-ROM probe for CoilMaster.
// It does not initialize or control SSR, motor, Hall sensor, UART jobs, or START.
// Upload this sketch only when you want to identify the LCD controller character set.

namespace
{
constexpr uint8_t LcdAddress = 0x27;
constexpr uint8_t LcdColumns = 16;
constexpr uint8_t LcdRows = 2;
constexpr unsigned long PageDurationMs = 3500UL;

LiquidCrystal_I2C lcd(LcdAddress, LcdColumns, LcdRows);
uint8_t page = 0;
unsigned long lastPageChangeMs = 0UL;

void printHexNibble(uint8_t value)
{
    value &= 0x0F;
    lcd.write(value < 10 ? static_cast<uint8_t>('0' + value)
                         : static_cast<uint8_t>('A' + value - 10));
}

void printHexByte(uint8_t value)
{
    printHexNibble(static_cast<uint8_t>(value >> 4));
    printHexNibble(value);
}

void renderIntro()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("LCD ROM TEST"));
    lcd.setCursor(0, 1);
    lcd.print(F("A0-FF: 3.5 sec"));
}

void renderPage(uint8_t pageIndex)
{
    // Six pages: A0-AF, B0-BF, ... F0-FF.
    const uint8_t base = static_cast<uint8_t>(0xA0U + pageIndex * 0x10U);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("ROM "));
    printHexByte(base);
    lcd.print('-');
    printHexByte(static_cast<uint8_t>(base + 0x0FU));

    lcd.setCursor(0, 1);
    for (uint8_t offset = 0; offset < 16; ++offset)
        lcd.write(static_cast<uint8_t>(base + offset));

    Serial.print(F("LCD_ROM_PAGE base=0x"));
    if (base < 0x10U) Serial.print('0');
    Serial.println(base, HEX);
}
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    lcd.init();
    lcd.backlight();

    Serial.println(F("================================"));
    Serial.println(F("CoilMaster LCD1602 Cyrillic probe"));
    Serial.println(F("No motor/SSR/START control is used."));
    Serial.println(F("Photograph pages A0..FF or note which bytes look Cyrillic."));
    Serial.println(F("If the LCD stays blank, verify I2C address (default 0x27)."));
    Serial.println(F("================================"));

    renderIntro();
    delay(2000);
    renderPage(page);
    lastPageChangeMs = millis();
}

void loop()
{
    const unsigned long now = millis();
    if (now - lastPageChangeMs < PageDurationMs)
        return;

    page = static_cast<uint8_t>((page + 1U) % 6U);
    renderPage(page);
    lastPageChangeMs = now;
}
