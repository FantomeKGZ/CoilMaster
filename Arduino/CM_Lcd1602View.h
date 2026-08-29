#ifndef CM_LCD1602_VIEW_H
#define CM_LCD1602_VIEW_H

#include "CM_Pcf8574Lcd.h"
#include "../Core/CM_UiModel.h"

#ifndef CM_LCD_RU_EN
#define CM_LCD_RU_EN 0
#endif

namespace CM
{

using LiquidCrystal_I2C = Pcf8574Lcd;

/**
 * @brief Arduino adapter that renders UiModel on a 16x2 I2C LCD.
 *
 * The class keeps compact hashes of the last rendered rows and only updates
 * changed rows. This avoids lcd.clear() flicker without keeping two complete
 * 17-byte row copies resident in the Uno's limited SRAM.
 */
class Lcd1602View
{
public:
    explicit Lcd1602View(Pcf8574Lcd& lcd);

    void begin();
    void render(const UiModel& model);
    void invalidate();

private:
    static constexpr uint8_t Columns = 16U;
    static constexpr uint8_t Rows = 2U;

    void buildLines(const UiModel& model,
                    char (&line1)[Columns + 1U],
                    char (&line2)[Columns + 1U]) const;

#if CM_LCD_RU_EN
    void prepareRussianGlyphs(UiScreen screen);
    void prepareHallRussianGlyphs();
    void loadRussianGlyph(uint8_t slot, const uint8_t* bitmap);
#endif

    static void clearLine(char (&line)[Columns + 1U]);
    static void copyPadded(char (&destination)[Columns + 1U],
                           const char* source);
    static void applySyncMarker(char (&line)[Columns + 1U],
                                const UiModel& model);
    static uint32_t lineHash(const char* line);
    void writeLine(uint8_t row, const char* line);

    Pcf8574Lcd& m_lcd;
    uint32_t m_lastLine1Hash;
    uint32_t m_lastLine2Hash;
    bool m_hasLastLine1;
    bool m_hasLastLine2;
    bool m_initialized;
#if CM_LCD_RU_EN
    UiScreen m_russianGlyphScreen;
    bool m_hasRussianGlyphScreen;
    bool m_hasHallRussianGlyphs;
#endif
};

} // namespace CM

#endif // CM_LCD1602_VIEW_H
