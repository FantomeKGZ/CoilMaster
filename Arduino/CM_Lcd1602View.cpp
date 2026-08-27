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

#if CM_LCD_RU_EN
// Only letters that do not have a visually compatible ASCII glyph are stored.
// Slots 1..7 are used; slot 0 is deliberately avoided because row buffers are
// zero-terminated C strings. Each screen needs at most five custom glyphs.
const uint8_t RuB[8] PROGMEM =
    {0x1EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x1EU, 0x00U};
const uint8_t RuG[8] PROGMEM =
    {0x1FU, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x00U};
const uint8_t RuD[8] PROGMEM =
    {0x0EU, 0x0AU, 0x0AU, 0x0AU, 0x0AU, 0x1FU, 0x11U, 0x00U};
const uint8_t RuZh[8] PROGMEM =
    {0x15U, 0x15U, 0x0EU, 0x04U, 0x0EU, 0x15U, 0x15U, 0x00U};
const uint8_t RuZ[8] PROGMEM =
    {0x0EU, 0x11U, 0x01U, 0x06U, 0x01U, 0x11U, 0x0EU, 0x00U};
const uint8_t RuI[8] PROGMEM =
    {0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x11U, 0x11U, 0x00U};
const uint8_t RuShortI[8] PROGMEM =
    {0x0AU, 0x04U, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x00U};
const uint8_t RuL[8] PROGMEM =
    {0x07U, 0x09U, 0x09U, 0x09U, 0x09U, 0x11U, 0x11U, 0x00U};
const uint8_t RuP[8] PROGMEM =
    {0x1FU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x00U};
const uint8_t RuU[8] PROGMEM =
    {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x08U, 0x10U, 0x00U};
const uint8_t RuCh[8] PROGMEM =
    {0x11U, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x01U, 0x00U};
const uint8_t RuSh[8] PROGMEM =
    {0x15U, 0x15U, 0x15U, 0x15U, 0x15U, 0x15U, 0x1FU, 0x00U};
const uint8_t RuSoft[8] PROGMEM =
    {0x10U, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x1EU, 0x00U};
#endif

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
#if CM_LCD_RU_EN
      , m_russianGlyphScreen(UiScreen::Fault),
      m_hasRussianGlyphScreen(false)
#endif
{
}

void Lcd1602View::begin()
{
    m_lcd.init();
    m_lcd.backlight();
    m_lcd.clear();
#if CM_LCD_RU_EN
    m_hasRussianGlyphScreen = false;
#endif
    invalidate();
    m_initialized = true;
}

void Lcd1602View::render(const UiModel& model)
{
    if (!m_initialized) return;

#if CM_FEATURE_DIAGNOSTICS
    traceRenderStage(F("LCD_RENDER_ENTER"));
#endif

#if CM_LCD_RU_EN
    prepareRussianGlyphs(model.screen);
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

#if CM_LCD_RU_EN
void Lcd1602View::loadRussianGlyph(uint8_t slot, const uint8_t* bitmap)
{
    uint8_t rows[8];
    for (uint8_t row = 0U; row < 8U; ++row)
        rows[row] = pgm_read_byte(bitmap + row);
    m_lcd.createChar(slot, rows);
}

void Lcd1602View::prepareRussianGlyphs(UiScreen screen)
{
    if (m_hasRussianGlyphScreen && m_russianGlyphScreen == screen) return;

    switch (screen)
    {
        case UiScreen::EnterCoilCount:
            loadRussianGlyph(1U, RuD);
            loadRussianGlyph(2U, RuZh);
            loadRussianGlyph(3U, RuI);
            loadRussianGlyph(4U, RuL);
            break;
        case UiScreen::EnterTurns:
            loadRussianGlyph(1U, RuD);
            loadRussianGlyph(2U, RuI);
            loadRussianGlyph(3U, RuZh);
            break;
        case UiScreen::Ready:
            loadRussianGlyph(1U, RuG);
            loadRussianGlyph(2U, RuU);
            loadRussianGlyph(3U, RuCh);
            break;
        case UiScreen::Winding:
            loadRussianGlyph(1U, RuI);
            break;
        case UiScreen::Paused:
            loadRussianGlyph(1U, RuZ);
            loadRussianGlyph(2U, RuI);
            loadRussianGlyph(3U, RuP);
            loadRussianGlyph(4U, RuU);
            break;
        case UiScreen::ManualRun:
            loadRussianGlyph(1U, RuZh);
            loadRussianGlyph(2U, RuShortI);
            loadRussianGlyph(3U, RuP);
            loadRussianGlyph(4U, RuU);
            loadRussianGlyph(5U, RuCh);
            break;
        case UiScreen::CoilComplete:
            loadRussianGlyph(1U, RuG);
            loadRussianGlyph(2U, RuD);
            loadRussianGlyph(3U, RuL);
            loadRussianGlyph(4U, RuSh);
            loadRussianGlyph(5U, RuSoft);
            break;
        case UiScreen::JobComplete:
            loadRussianGlyph(1U, RuG);
            loadRussianGlyph(2U, RuP);
            break;
        case UiScreen::Fault:
        default:
            loadRussianGlyph(1U, RuB);
            loadRussianGlyph(2U, RuI);
            loadRussianGlyph(3U, RuSh);
            break;
    }

    m_russianGlyphScreen = screen;
    m_hasRussianGlyphScreen = true;
    invalidate();
}
#endif

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
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("KO" "\x04" "-BO KAT.?"));
            if (model.inputDigits > 0U)
            {
                appendFlash(line2, p2, F("BBO" "\x01" ":"));
                appendUnsigned(line2, p2, model.inputValue);
                appendFlash(line2, p2, F("  #=OK"));
            }
            else
            {
                appendFlash(line2, p2,
                            F("BBO" "\x01" " " "\x03" " HA" "\x02" "M" "\x03" " #"));
            }
#else
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
#endif
            break;

        case UiScreen::EnterTurns:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("B" "\x02" "TK" "\x02" " "));
#else
            appendFlash(line1, p1, F("VITKI "));
#endif
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
            if (model.inputDigits > 0U)
            {
#if CM_LCD_RU_EN
                appendFlash(line2, p2, F("BBO" "\x01" ":"));
#else
                appendFlash(line2, p2, F("VVOD:"));
#endif
                appendUnsigned(line2, p2, model.inputValue);
                appendFlash(line2, p2, F("  #=OK"));
            }
            else
            {
#if CM_LCD_RU_EN
                appendFlash(line2, p2,
                            F("BBO" "\x01" " " "\x02" " HA" "\x03" "M" "\x02" " #"));
#else
                appendFlash(line2, p2, F("VVOD I NAZHMI #"));
#endif
            }
            break;

        case UiScreen::Ready:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("\x01" "OTOB "));
#else
            appendFlash(line1, p1, F("GOTOV "));
#endif
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("A=CTAPT C=P" "\x02" "\x03"));
#else
            appendFlash(line2, p2, F("A=START C=RUCH"));
#endif
            break;

        case UiScreen::Winding:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("HAMOTKA "));
#else
            appendFlash(line1, p1, F("NAMOTKA "));
#endif
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("B" "\x01" "TK" "\x01" ":"));
#else
            appendFlash(line2, p2, F("VITKI:"));
#endif
            appendUnsigned(line2, p2, model.currentTurns);
            appendFlash(line2, p2, F("/"));
            appendUnsigned(line2, p2, model.targetTurns);
            break;

        case UiScreen::Paused:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("\x03" "A" "\x04" "\x01" "A "));
#else
            appendFlash(line1, p1, F("PAUZA "));
#endif
            appendUnsigned(line1, p1, model.coilNumber);
            appendFlash(line1, p1, F("/"));
            appendUnsigned(line1, p1, model.coilCount);
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("B" "\x02" "TK" "\x02" ":"));
#else
            appendFlash(line2, p2, F("VITKI:"));
#endif
            appendUnsigned(line2, p2, model.currentTurns);
            appendFlash(line2, p2, F("/"));
            appendUnsigned(line2, p2, model.targetTurns);
            appendFlash(line2, p2, F(" A=GO"));
            break;

        case UiScreen::ManualRun:
#if CM_LCD_RU_EN
            appendFlash(line1, p1,
                        F("P" "\x04" "\x05" "HO" "\x02" " PE" "\x01" "."));
            appendFlash(line2, p2, F("C=CTO" "\x03"));
#else
            appendFlash(line1, p1, F("RUCHNOY REZH."));
            appendFlash(line2, p2, F("C=STOP"));
#endif
            break;

        case UiScreen::CoilComplete:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("KAT. " "\x01" "OTOBA"));
            appendFlash(line2, p2,
                        F("A=" "\x02" "A" "\x03" "\x05" "\x04" "E"));
#else
            appendFlash(line1, p1, F("KAT. GOTOVA"));
            appendFlash(line2, p2, F("A=DAL'SHE"));
#endif
            break;

        case UiScreen::JobComplete:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("\x01" "OTOBO "));
#else
            appendFlash(line1, p1, F("GOTOVO "));
#endif
            appendUnsigned(line1, p1, model.completedRuns);
            appendFlash(line1, p1, F("X"));
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("A=" "\x02" "OBTOP B=MENU"));
#else
            appendFlash(line2, p2, F("A=POVTOR B=MENU"));
#endif
            break;

        case UiScreen::Fault:
        default:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("O" "\x03" "\x02" "\x01" "KA"));
            appendFlash(line2, p2, F("B=C" "\x01" "POC"));
#else
            appendFlash(line1, p1, F("OSHIBKA"));
            appendFlash(line2, p2, F("B=SBROS"));
#endif
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
