#include "CM_Lcd1602View.h"

#include <stdio.h>
#include <string.h>

namespace CM
{

Lcd1602View::Lcd1602View(LiquidCrystal_I2C& lcd)
    : m_lcd(lcd),
      m_lastLine1(),
      m_lastLine2(),
      m_initialized(false)
{
    invalidate();
}

void Lcd1602View::begin()
{
    m_lcd.init();
    m_lcd.backlight();
    m_lcd.clear();
    invalidate();
    m_initialized = true;
}

void Lcd1602View::render(const UiModel& model)
{
    if (!m_initialized)
    {
        return;
    }

    char line1[Columns + 1U];
    char line2[Columns + 1U];
    buildLines(model, line1, line2);

    if (strncmp(line1, m_lastLine1, Columns) != 0)
    {
        writeLine(0U, line1);
        memcpy(m_lastLine1, line1, Columns + 1U);
    }

    if (strncmp(line2, m_lastLine2, Columns) != 0)
    {
        writeLine(1U, line2);
        memcpy(m_lastLine2, line2, Columns + 1U);
    }
}

void Lcd1602View::invalidate()
{
    memset(m_lastLine1, 0, sizeof(m_lastLine1));
    memset(m_lastLine2, 0, sizeof(m_lastLine2));
}

void Lcd1602View::buildLines(const UiModel& model,
                             char (&line1)[Columns + 1U],
                             char (&line2)[Columns + 1U]) const
{
    char buffer1[32];
    char buffer2[32];
    buffer1[0] = '\0';
    buffer2[0] = '\0';

    switch (model.screen)
    {
        case UiScreen::EnterCoilCount:
            snprintf(buffer1, sizeof(buffer1), "KOL-VO KATUSHEK?");
            if (model.inputDigits > 0U)
            {
                snprintf(buffer2, sizeof(buffer2), "VVOD:%u  #=OK",
                         static_cast<unsigned int>(model.inputValue));
            }
            else
            {
                snprintf(buffer2, sizeof(buffer2), "VVOD I NAZHMI #");
            }
            break;

        case UiScreen::EnterTurns:
            snprintf(buffer1, sizeof(buffer1), "VITKI KAT.%u/%u",
                     static_cast<unsigned int>(model.coilNumber),
                     static_cast<unsigned int>(model.coilCount));
            if (model.inputDigits > 0U)
            {
                snprintf(buffer2, sizeof(buffer2), "VVOD:%u  #=OK",
                         static_cast<unsigned int>(model.inputValue));
            }
            else
            {
                snprintf(buffer2, sizeof(buffer2), "VVOD I NAZHMI #");
            }
            break;

        case UiScreen::Ready:
            snprintf(buffer1, sizeof(buffer1), "GOTOVO KAT.%u/%u",
                     static_cast<unsigned int>(model.coilNumber),
                     static_cast<unsigned int>(model.coilCount));
            snprintf(buffer2, sizeof(buffer2), "A=START C=RUCH");
            break;

        case UiScreen::Winding:
            snprintf(buffer1, sizeof(buffer1), "NAMOTKA KAT.%u/%u",
                     static_cast<unsigned int>(model.coilNumber),
                     static_cast<unsigned int>(model.coilCount));
            snprintf(buffer2, sizeof(buffer2), "VITKI:%u/%u",
                     static_cast<unsigned int>(model.currentTurns),
                     static_cast<unsigned int>(model.targetTurns));
            break;

        case UiScreen::Paused:
            snprintf(buffer1, sizeof(buffer1), "PAUZA KAT.%u/%u",
                     static_cast<unsigned int>(model.coilNumber),
                     static_cast<unsigned int>(model.coilCount));
            snprintf(buffer2, sizeof(buffer2), "VITKI:%u/%u A=GO",
                     static_cast<unsigned int>(model.currentTurns),
                     static_cast<unsigned int>(model.targetTurns));
            break;

        case UiScreen::ManualRun:
            snprintf(buffer1, sizeof(buffer1), "RUCHNOY REZHIM");
            snprintf(buffer2, sizeof(buffer2), "C=STOP");
            break;

        case UiScreen::CoilComplete:
            snprintf(buffer1, sizeof(buffer1), "KATUSHKA GOTOVA");
            snprintf(buffer2, sizeof(buffer2), "A=DAL'SHE");
            break;

        case UiScreen::JobComplete:
            snprintf(buffer1, sizeof(buffer1), "NAMOTKA GOTOVA");
            snprintf(buffer2, sizeof(buffer2), "B=V MENYU");
            break;

        case UiScreen::Fault:
        default:
            snprintf(buffer1, sizeof(buffer1), "OSHIBKA SISTEMY");
            snprintf(buffer2, sizeof(buffer2), "B=SBROS");
            break;
    }

    copyPadded(line1, buffer1);
    copyPadded(line2, buffer2);
}

void Lcd1602View::clearLine(char (&line)[Columns + 1U])
{
    for (uint8_t index = 0U; index < Columns; ++index)
    {
        line[index] = ' ';
    }
    line[Columns] = '\0';
}

void Lcd1602View::copyPadded(char (&destination)[Columns + 1U],
                             const char* source)
{
    clearLine(destination);

    if (source == nullptr)
    {
        return;
    }

    const size_t length = strlen(source);
    const size_t copyLength = length < Columns ? length : Columns;
    memcpy(destination, source, copyLength);
}

void Lcd1602View::writeLine(uint8_t row, const char* line)
{
    m_lcd.setCursor(0U, row);
    m_lcd.print(line);
}

} // namespace CM
