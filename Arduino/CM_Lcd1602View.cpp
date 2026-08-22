#include "CM_Lcd1602View.h"
#include "Config/CM_Features.h"

#include <Arduino.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#endif

namespace
{
constexpr uint8_t DisplayColumns = 16U;
constexpr uint8_t SyncMarkerStart = 13U;
static_assert(SyncMarkerStart + 3U == DisplayColumns,
              "LCD sync marker must occupy the final three columns");

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
      m_lastLine1Hash(0UL),
      m_lastLine2Hash(0UL),
      m_hasLastLine1(false),
      m_hasLastLine2(false),
      m_initialized(false)
{
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

    const uint32_t hash1 = lineHash(line1);
    const uint32_t hash2 = lineHash(line2);

    if (!m_hasLastLine1 || hash1 != m_lastLine1Hash)
    {
        writeLine(0U, line1);
        m_lastLine1Hash = hash1;
        m_hasLastLine1 = true;
    }

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_ROW1"));
#endif

    if (!m_hasLastLine2 || hash2 != m_lastLine2Hash)
    {
        writeLine(1U, line2);
        m_lastLine2Hash = hash2;
        m_hasLastLine2 = true;
    }

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_ROW2"));
    traceRenderStage(F("LCD_RENDERED"));
    firstRenderTrace = false;
#endif
}

void Lcd1602View::invalidate()
{
    m_lastLine1Hash = 0UL;
    m_lastLine2Hash = 0UL;
    m_hasLastLine1 = false;
    m_hasLastLine2 = false;
}

void Lcd1602View::buildLines(const UiModel& model,
                             char (&line1)[Columns + 1U],
                             char (&line2)[Columns + 1U]) const
{
    clearLine(line1);
    clearLine(line2);
    uint8_t p1 = 0U;
    uint8_t p2 = 0U;

    // Columns 13..15 of row 1 are reserved for the synchronization marker.
    // Keep every row-1 label compact enough that the marker never destroys
    // operator-facing text. MaxCoilsPerJob is 10, so X/Y uses at most 5 chars.
    switch (model.screen)
    {
        case UiScreen::EnterCoilCount:
            appendFlash(line1, p1, F("KOL-VO KAT.?"));
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
            appendFlash(line1, p1, F("VITKI "));
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
            appendFlash(line1, p1, F("GOTOV "));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            appendFlash(line2, p2, F("A=START C=RUCH"));
            break;

        case UiScreen::Winding:
            appendFlash(line1, p1, F("NAMOTKA "));
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            appendFlash(line2, p2, F("VITKI:"));
            appendUnsigned(line2, p2, model.currentTurns);
            appendFlash(line2, p2, F("/"));
            appendUnsigned(line2, p2, model.targetTurns);
            break;

        case UiScreen::Paused:
            appendFlash(line1, p1, F("PAUZA "));
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
            appendFlash(line1, p1, F("RUCHNOY REZH."));
            appendFlash(line2, p2, F("C=STOP"));
            break;

        case UiScreen::CoilComplete:
            appendFlash(line1, p1, F("KAT. GOTOVA"));
            appendFlash(line2, p2, F("A=DAL'SHE"));
            break;

        case UiScreen::JobComplete:
            appendFlash(line1, p1, F("GOTOVO "));
            appendUnsigned(line1, p1, model.completedRuns);
            appendFlash(line1, p1, F("X"));
            appendFlash(line2, p2, F("A=POVTOR B=MENU"));
            break;

        case UiScreen::Fault:
        default:
            appendFlash(line1, p1, F("OSHIBKA"));
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

    line[SyncMarkerStart] = ' ';
    if (model.syncState == UiSyncState::Error)
    {
        line[SyncMarkerStart + 1U] = 'E';
        line[SyncMarkerStart + 2U] = static_cast<char>('0' + displayedCount);
    }
    else if (model.syncState == UiSyncState::Pending ||
             model.pendingSyncCount > 0U)
    {
        line[SyncMarkerStart + 1U] = 'P';
        line[SyncMarkerStart + 2U] = static_cast<char>('0' + displayedCount);
    }
    else
    {
        line[SyncMarkerStart + 1U] = 'O';
        line[SyncMarkerStart + 2U] = 'K';
    }
}

uint32_t Lcd1602View::lineHash(const char* line)
{
    if (line == nullptr) return 0UL;

    uint32_t hash = 2166136261UL;
    for (uint8_t index = 0U; index < Columns; ++index)
    {
        hash ^= static_cast<uint8_t>(line[index]);
        hash *= 16777619UL;
    }
    return hash;
}

void Lcd1602View::writeLine(uint8_t row, const char* line)
{
    m_lcd.setCursor(0U, row);
    m_lcd.print(line);
}

} // namespace CM
