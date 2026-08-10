#include "CM_Lcd1602View.h"
#include "Config/CM_Features.h"

#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#endif

namespace
{
constexpr uint8_t DisplayColumns = 16U;

#if CM_FEATURE_DIAGNOSTICS
bool firstRenderTrace = true;

void traceRenderStage(const __FlashStringHelper* stage)
{
    if (!firstRenderTrace) return;
    Serial.print(F("CM_BOOT stage="));
    Serial.println(stage);
    Serial.flush();
}
#endif

void appendFlash(char (&line)[DisplayColumns + 1U],
                 uint8_t& position,
                 const __FlashStringHelper* text)
{
    if (text == nullptr) return;

#if defined(__AVR__)
    const char* cursor = reinterpret_cast<const char*>(text);
    while (position < DisplayColumns)
    {
        const char value = static_cast<char>(pgm_read_byte(cursor++));
        if (value == '\0') break;
        line[position++] = value;
    }
#else
    const char* cursor = reinterpret_cast<const char*>(text);
    while (position < DisplayColumns && *cursor != '\0')
        line[position++] = *cursor++;
#endif
}

void appendUnsigned(char (&line)[DisplayColumns + 1U],
                    uint8_t& position,
                    uint16_t value)
{
    char digits[5];
    uint8_t count = 0U;
    do
    {
        digits[count++] = static_cast<char>('0' + (value % 10U));
        value = static_cast<uint16_t>(value / 10U);
    }
    while (value != 0U && count < sizeof(digits));

    while (count > 0U && position < DisplayColumns)
        line[position++] = digits[--count];
}
}

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
    if (!m_initialized) return;

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_RENDER_ENTER"));
#endif

    char line1[Columns + 1U];
    char line2[Columns + 1U];
    buildLines(model, line1, line2);

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_LINES_BUILT"));
#endif

    if (strncmp(line1, m_lastLine1, Columns) != 0)
    {
        writeLine(0U, line1);
        memcpy(m_lastLine1, line1, Columns + 1U);
    }

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_ROW1"));
#endif

    if (strncmp(line2, m_lastLine2, Columns) != 0)
    {
        writeLine(1U, line2);
        memcpy(m_lastLine2, line2, Columns + 1U);
    }

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_ROW2"));
    traceRenderStage(F("LCD_RENDERED"));
    firstRenderTrace = false;
#endif
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
    clearLine(line1);
    clearLine(line2);
    uint8_t p1 = 0U;
    uint8_t p2 = 0U;

    switch (model.screen)
    {
        case UiScreen::EnterCoilCount:
            appendFlash(line1, p1, F("KOL-VO KATUSHEK?"));
            if (model.inputDigits > 0U)
            {
                appendFlash(line2, p2, F("VVOD:"));
                appendUnsigned(line2, p2, model.inputValue);
                appendFlash(line2, p2, F("  #=OK"));
            }
            else
            {
                appendFlash(line2, p2, F("VVOD I NAZHMI #"));
            }
            break;

        case UiScreen::EnterTurns:
            appendFlash(line1, p1, F("VITKI KAT."));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            if (model.inputDigits > 0U)
            {
                appendFlash(line2, p2, F("VVOD:"));
                appendUnsigned(line2, p2, model.inputValue);
                appendFlash(line2, p2, F("  #=OK"));
            }
            else
            {
                appendFlash(line2, p2, F("VVOD I NAZHMI #"));
            }
            break;

        case UiScreen::Ready:
            appendFlash(line1, p1, F("GOTOVO KAT."));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            appendFlash(line2, p2, F("A=START C=RUCH"));
            break;

        case UiScreen::Winding:
            appendFlash(line1, p1, F("NAMOTKA KAT."));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            appendFlash(line2, p2, F("VITKI:"));
            appendUnsigned(line2, p2, model.currentTurns);
            appendFlash(line2, p2, F("/"));
            appendUnsigned(line2, p2, model.targetTurns);
            break;

        case UiScreen::Paused:
            appendFlash(line1, p1, F("PAUZA KAT."));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            appendFlash(line2, p2, F("VITKI:"));
            appendUnsigned(line2, p2, model.currentTurns);
            appendFlash(line2, p2, F("/"));
            appendUnsigned(line2, p2, model.targetTurns);
            appendFlash(line2, p2, F(" A=GO"));
            break;

        case UiScreen::ManualRun:
            appendFlash(line1, p1, F("RUCHNOY REZHIM"));
            appendFlash(line2, p2, F("C=STOP"));
            break;

        case UiScreen::CoilComplete:
            appendFlash(line1, p1, F("KATUSHKA GOTOVA"));
            appendFlash(line2, p2, F("A=DAL'SHE"));
            break;

        case UiScreen::JobComplete:
            appendFlash(line1, p1, F("GOTOVO: "));
            appendUnsigned(line1, p1, model.completedRuns);
            appendFlash(line1, p1, F(" RAZ"));
            appendFlash(line2, p2, F("A=POVTOR B=MENU"));
            break;

        case UiScreen::Fault:
        default:
            appendFlash(line1, p1, F("OSHIBKA SISTEMY"));
            appendFlash(line2, p2, F("B=SBROS"));
            break;
    }

    applySyncMarker(line1, model);
}

void Lcd1602View::clearLine(char (&line)[Columns + 1U])
{
    for (uint8_t index = 0U; index < Columns; ++index)
        line[index] = ' ';
    line[Columns] = '\0';
}

void Lcd1602View::copyPadded(char (&destination)[Columns + 1U],
                             const char* source)
{
    clearLine(destination);
    if (source == nullptr) return;

    const size_t length = strlen(source);
    const size_t copyLength = length < Columns ? length : Columns;
    memcpy(destination, source, copyLength);
}

void Lcd1602View::applySyncMarker(char (&line)[Columns + 1U],
                                  const UiModel& model)
{
    const uint8_t displayedCount = model.pendingSyncCount > 9U
                                       ? 9U
                                       : model.pendingSyncCount;

    line[13] = ' ';
    if (model.syncState == UiSyncState::Error)
    {
        line[14] = 'E';
        line[15] = static_cast<char>('0' + displayedCount);
    }
    else if (model.syncState == UiSyncState::Pending ||
             model.pendingSyncCount > 0U)
    {
        line[14] = 'P';
        line[15] = static_cast<char>('0' + displayedCount);
    }
    else
    {
        line[14] = 'O';
        line[15] = 'K';
    }
}

void Lcd1602View::writeLine(uint8_t row, const char* line)
{
    m_lcd.setCursor(0U, row);
    m_lcd.print(line);
}

} // namespace CM
