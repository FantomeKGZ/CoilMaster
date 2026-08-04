#ifndef CM_LCD1602_VIEW_H
#define CM_LCD1602_VIEW_H

#include <LiquidCrystal_I2C.h>

#include "../Core/CM_UiModel.h"

namespace CM
{

/**
 * @brief Arduino adapter that renders UiModel on a 16x2 I2C LCD.
 *
 * The class keeps the last rendered lines and only updates changed rows.
 * This avoids lcd.clear() flicker and unnecessary I2C traffic.
 */
class Lcd1602View
{
public:
    explicit Lcd1602View(LiquidCrystal_I2C& lcd);

    void begin();
    void render(const UiModel& model);
    void invalidate();

private:
    static constexpr uint8_t Columns = 16U;
    static constexpr uint8_t Rows = 2U;

    void buildLines(const UiModel& model,
                    char (&line1)[Columns + 1U],
                    char (&line2)[Columns + 1U]) const;

    static void clearLine(char (&line)[Columns + 1U]);
    static void copyPadded(char (&destination)[Columns + 1U],
                           const char* source);
    void writeLine(uint8_t row, const char* line);

    LiquidCrystal_I2C& m_lcd;
    char m_lastLine1[Columns + 1U];
    char m_lastLine2[Columns + 1U];
    bool m_initialized;
};

} // namespace CM

#endif // CM_LCD1602_VIEW_H
