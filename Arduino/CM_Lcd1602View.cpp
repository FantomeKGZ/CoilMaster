#include "CM_Lcd1602View.h"
#include "Config/CM_Features.h"
#if CM_LCD_RU_EN
#include "CM_HallCalibrationService.h"
#endif

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
enum RuGlyph : uint8_t
{
    RuNone = 0U,
    RuB,
    RuG,
    RuD,
    RuZh,
    RuZ,
    RuI,
    RuShortI,
    RuL,
    RuP,
    RuU,
    RuCh,
    RuSh,
    RuSoft
};

const uint8_t RuGlyphs[][8] PROGMEM =
{
    {0x1EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x1EU, 0x00U}, // Б
    {0x1FU, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x00U}, // Г
    {0x0EU, 0x0AU, 0x0AU, 0x0AU, 0x0AU, 0x1FU, 0x11U, 0x00U}, // Д
    {0x15U, 0x15U, 0x0EU, 0x04U, 0x0EU, 0x15U, 0x15U, 0x00U}, // Ж
    {0x0EU, 0x11U, 0x01U, 0x06U, 0x01U, 0x11U, 0x0EU, 0x00U}, // З
    {0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x11U, 0x11U, 0x00U}, // И
    {0x0AU, 0x04U, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x00U}, // Й
    {0x07U, 0x09U, 0x09U, 0x09U, 0x09U, 0x11U, 0x11U, 0x00U}, // Л
    {0x1FU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x00U}, // П
    {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x08U, 0x10U, 0x00U}, // У
    {0x11U, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x01U, 0x00U}, // Ч
    {0x15U, 0x15U, 0x15U, 0x15U, 0x15U, 0x15U, 0x1FU, 0x00U}, // Ш
    {0x10U, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x1EU, 0x00U}  // Ь
};

constexpr uint8_t RuGlyphSlotsPerScreen = 5U;
const uint8_t RuScreenGlyphs[][RuGlyphSlotsPerScreen] PROGMEM =
{
    {RuD,  RuZh,     RuI, RuL,  RuNone}, // EnterCoilCount
    {RuD,  RuI,      RuZh, RuNone, RuNone}, // EnterTurns
    {RuG,  RuU,      RuCh, RuNone, RuNone}, // Ready
    {RuI,  RuNone,   RuNone, RuNone, RuNone}, // Winding
    {RuZ,  RuI,      RuP, RuU, RuNone}, // Paused
    {RuZh, RuShortI, RuP, RuU, RuCh}, // ManualRun
    {RuG,  RuD,      RuL, RuSh, RuSoft}, // CoilComplete
    {RuG,  RuP,      RuNone, RuNone, RuNone}, // JobComplete
    {RuB,  RuI,      RuSh, RuNone, RuNone} // Fault
};

static_assert(static_cast<uint8_t>(CM::UiScreen::Fault) + 1U ==
                  sizeof(RuScreenGlyphs) / sizeof(RuScreenGlyphs[0]),
              "RU glyph table must cover every UiScreen");
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
    char digits[6];
    utoa(value, digits, 10);
    const char* cursor = digits;
    while (position < DisplayColumns && *cursor != '\0')
        line[position++] = *cursor++;
}

void appendNumberPair(char (&line)[DisplayColumns + 1U],
                      uint8_t& position,
                      uint16_t first,
                      uint16_t second)
{
    appendUnsigned(line, position, first);
    if (position < DisplayColumns) line[position++] = '/';
    appendUnsigned(line, position, second);
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

    char line1[Columns + 1U];
    char line2[Columns + 1U];

#if CM_LCD_RU_EN
    const HallCalibrationState calibrationState = HallCalibrationService::displayState();
    if (calibrationState == HallCalibrationState::WaitingLocalConfirm)
    {
        copyPadded(line1, "HALL TEST");
        copyPadded(line2, "#=OK B=CANCEL");
    }
    else if (calibrationState == HallCalibrationState::ArmedWaitingPhysicalStart)
    {
        copyPadded(line1, "HALL TEST READY");
        copyPadded(line2, "A OR START");
    }
    else if (calibrationState == HallCalibrationState::Running)
    {
        copyPadded(line1, "HALL TEST RUN");
        clearLine(line2);
        uint8_t position = 0U;
        appendFlash(line2, position, F("LEFT "));
        const uint32_t elapsed = static_cast<uint32_t>(millis() -
            HallCalibrationService::displayStartedAtMs());
        const uint32_t duration = HallCalibrationService::displayRunDurationMs();
        const uint16_t seconds = static_cast<uint16_t>(
            elapsed >= duration ? 0UL : (duration - elapsed + 999UL) / 1000UL);
        appendUnsigned(line2, position, seconds);
        appendFlash(line2, position, F(" SEC"));
    }
    else if (calibrationState == HallCalibrationState::WaitingApplyConfirm)
    {
        copyPadded(line1, "SAVE HALL CFG?");
        copyPadded(line2, "#=YES B=NO");
    }
    else
    {
        prepareRussianGlyphs(model.screen);
        buildLines(model, line1, line2);
    }
#else
    buildLines(model, line1, line2);
#endif

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
    m_lcd.createChar_P(slot, bitmap);
}

void Lcd1602View::prepareRussianGlyphs(UiScreen screen)
{
    if (m_hasRussianGlyphScreen && m_russianGlyphScreen == screen) return;

    const uint8_t screenIndex = static_cast<uint8_t>(screen);
    for (uint8_t index = 0U; index < RuGlyphSlotsPerScreen; ++index)
    {
        const uint8_t glyph = pgm_read_byte(&RuScreenGlyphs[screenIndex][index]);
        if (glyph == RuNone) break;
        loadRussianGlyph(static_cast<uint8_t>(index + 1U), RuGlyphs[glyph - 1U]);
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
            appendNumberPair(line1, p1, model.coilNumber, model.coilCount);
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
            appendNumberPair(line1, p1, model.coilNumber, model.coilCount);
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
            appendNumberPair(line1, p1, model.coilNumber, model.coilCount);
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("B" "\x01" "TK" "\x01" ":"));
#else
            appendFlash(line2, p2, F("VITKI:"));
#endif
            appendNumberPair(line2, p2, model.currentTurns, model.targetTurns);
            break;

        case UiScreen::Paused:
#if CM_LCD_RU_EN
            appendFlash(line1, p1, F("\x03" "A" "\x04" "\x01" "A "));
#else
            appendFlash(line1, p1, F("PAUZA "));
#endif
            appendNumberPair(line1, p1, model.coilNumber, model.coilCount);
#if CM_LCD_RU_EN
            appendFlash(line2, p2, F("B" "\x02" "TK" "\x02" ":"));
#else
            appendFlash(line2, p2, F("VITKI:"));
#endif
            appendNumberPair(line2, p2, model.currentTurns, model.targetTurns);
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