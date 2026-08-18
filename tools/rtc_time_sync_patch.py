from pathlib import Path

main_path = Path("firmware/esp32/src/main.cpp")
text = main_path.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly 1 match, got {count}")
    text = text.replace(old, new, 1)


replace_once(
    '#include <Wire.h>\n#include <esp_heap_caps.h>',
    '#include <Wire.h>\n#include <time.h>\n#include <esp_heap_caps.h>',
    "time include",
)

replace_once(
    'constexpr int8_t RtcSdaPin = 21;\nconstexpr int8_t RtcSclPin = 22;\nconstexpr char AccessPointName[] = "CoilMaster";',
    'constexpr int8_t RtcSdaPin = 21;\n'
    'constexpr int8_t RtcSclPin = 22;\n'
    'constexpr char BishkekTimezoneName[] = "Asia/Bishkek";\n'
    'constexpr char BishkekPosixTimezone[] = "KGT-6";\n'
    'constexpr char NtpServer1[] = "pool.ntp.org";\n'
    'constexpr char NtpServer2[] = "time.google.com";\n'
    'constexpr char NtpServer3[] = "time.cloudflare.com";\n'
    'constexpr uint32_t NtpInitialWaitMs = 2000UL;\n'
    'constexpr uint32_t NtpRetryIntervalMs = 30000UL;\n'
    'constexpr uint32_t NtpRefreshIntervalMs = 21600000UL;\n'
    'constexpr char AccessPointName[] = "CoilMaster";',
    "RTC/NTP constants",
)

replace_once(
    'bool webRecoveryRequired = false;\nbool mdnsReady = false;\nesp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;',
    'bool webRecoveryRequired = false;\n'
    'bool mdnsReady = false;\n'
    'bool ntpConfigured = false;\n'
    'bool ntpSyncedThisConnection = false;\n'
    'bool ntpInitialAttemptPending = false;\n'
    'uint32_t lastNtpAttemptMs = 0UL;\n'
    'uint32_t lastNtpSyncMs = 0UL;\n'
    'const char* lastNtpSyncResult = "NOT_CONNECTED";\n'
    'esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;',
    "NTP state",
)

canonical_parser = '''bool parseCanonicalUint32(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    value = parsed;
    return true;
}
'''
rtc_parser = canonical_parser + '''
bool parseRtcDecimal(const String& source, uint32_t& value)
{
    // RTC fields are normally zero-padded (08, 05, 00). Keep the strict
    // canonical parser for IDs and allow leading zeroes only for date/time.
    value = 0UL;
    if (source.length() == 0U) return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    value = parsed;
    return true;
}
'''
replace_once(canonical_parser, rtc_parser, "RTC decimal parser")

same_program = '''bool sameProgram(const CM::OutgoingWindingJob& left,
                 const CM::OutgoingWindingJob& right)
{
    if (left.coilCount != right.coilCount) return false;
    for (uint8_t index = 0U; index < left.coilCount; ++index)
    {
        if (left.turns[index] != right.turns[index]) return false;
    }
    return true;
}
'''
ntp_updater = same_program + '''
void updateBishkekTimeSync(uint32_t nowMs)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        ntpConfigured = false;
        ntpSyncedThisConnection = false;
        ntpInitialAttemptPending = false;
        lastNtpAttemptMs = 0UL;
        lastNtpSyncResult = "NOT_CONNECTED";
        return;
    }

    if (!ntpConfigured)
    {
        // POSIX TZ signs are reversed: KGT-6 means local time = UTC+6.
        configTzTime(BishkekPosixTimezone, NtpServer1, NtpServer2, NtpServer3);
        ntpConfigured = true;
        ntpSyncedThisConnection = false;
        ntpInitialAttemptPending = true;
        lastNtpAttemptMs = nowMs;
        lastNtpSyncResult = "WAITING_NTP";
        Serial.println(F("NTP: configured Asia/Bishkek (UTC+6), waiting for network time"));
        return;
    }

    const uint32_t waitMs = ntpInitialAttemptPending
        ? NtpInitialWaitMs
        : (ntpSyncedThisConnection ? NtpRefreshIntervalMs : NtpRetryIntervalMs);
    if (static_cast<uint32_t>(nowMs - lastNtpAttemptMs) < waitMs) return;
    lastNtpAttemptMs = nowMs;
    ntpInitialAttemptPending = false;

    struct tm localTime = {};
    if (!getLocalTime(&localTime, 75U))
    {
        lastNtpSyncResult = "NTP_UNAVAILABLE";
        return;
    }

    const uint16_t year = static_cast<uint16_t>(localTime.tm_year + 1900);
    if (year < 2024U || year > 2099U)
    {
        lastNtpSyncResult = "NTP_TIME_INVALID";
        return;
    }

    // Do not change persistent wall-clock time during an active or uncertain
    // physical operation. Retry automatically once the system is safely idle.
    if (backupRuntimeActivity() != CM::BackupActivityCheck::Safe)
    {
        lastNtpSyncResult = "WAITING_SAFE_IDLE";
        return;
    }

    CM::RtcDateTime value;
    value.year = year;
    value.month = static_cast<uint8_t>(localTime.tm_mon + 1);
    value.day = static_cast<uint8_t>(localTime.tm_mday);
    value.hour = static_cast<uint8_t>(localTime.tm_hour);
    value.minute = static_cast<uint8_t>(localTime.tm_min);
    value.second = static_cast<uint8_t>(localTime.tm_sec);

    if (!rtcClock.set(value))
    {
        lastNtpSyncResult = rtcClock.detected()
            ? "RTC_WRITE_VERIFY_FAILED" : "RTC_NOT_DETECTED";
        Serial.print(F("NTP: DS3231 sync failed: "));
        Serial.println(lastNtpSyncResult);
        return;
    }

    ntpSyncedThisConnection = true;
    lastNtpSyncMs = nowMs;
    lastNtpSyncResult = "SYNCED";
    Serial.printf("NTP: DS3231 synchronized to Bishkek %04u-%02u-%02u %02u:%02u:%02u\\n",
                  static_cast<unsigned>(value.year),
                  static_cast<unsigned>(value.month),
                  static_cast<unsigned>(value.day),
                  static_cast<unsigned>(value.hour),
                  static_cast<unsigned>(value.minute),
                  static_cast<unsigned>(value.second));
}
'''
replace_once(same_program, ntp_updater, "NTP updater")

replace_once(
    '        response += F(",\\\"timezone_configured\\\":false,\\\"scheduling_ready\\\":false}");',
    '        response += F(",\\\"timezone\\\":\\\""); response += BishkekTimezoneName;\n'
    '        response += F("\\\",\\\"utc_offset_minutes\\\":360,\\\"timezone_configured\\\":true");\n'
    '        response += F(",\\\"ntp_enabled\\\":true,\\\"sta_connected\\\":");\n'
    '        response += WiFi.status() == WL_CONNECTED ? F("true") : F("false");\n'
    '        response += F(",\\\"ntp_synchronized\\\":");\n'
    '        response += ntpSyncedThisConnection ? F("true") : F("false");\n'
    '        response += F(",\\\"ntp_status\\\":\\\""); response += lastNtpSyncResult;\n'
    '        response += F("\\\",\\\"last_ntp_sync_uptime_ms\\\":"); response += lastNtpSyncMs;\n'
    '        response += F(",\\\"scheduling_ready\\\":false}");',
    "time GET status",
)

replace_once(
    '!parseCanonicalUint32(webServer.arg(fields[i]), parsed[i]))',
    '!parseRtcDecimal(webServer.arg(fields[i]), parsed[i]))',
    "RTC POST parser use",
)

replace_once(
    '    networkManager.update(nowMs);\n    webServer.handleClient();',
    '    networkManager.update(nowMs);\n    updateBishkekTimeSync(nowMs);\n    webServer.handleClient();',
    "loop time sync",
)

main_path.write_text(text, encoding="utf-8")

doc_path = Path("docs/HARDWARE_REFERENCE/04_RTC_TIME_SYNC.md")
if doc_path.exists():
    raise SystemExit("04_RTC_TIME_SYNC.md already exists unexpectedly")
doc_path.write_text(
    '''# DS3231 — время и автоматическая синхронизация Бишкек

Дата: **2026-08-18**  
Source of truth: `cmp-protocol-v1`

## Подключение

DS3231 подключен к ESP32 по I²C:

```text
DS3231 SDA -> ESP32 GPIO21
DS3231 SCL -> ESP32 GPIO22
GND        -> GND
VCC        -> питание модуля согласно его плате
```

Адрес DS3231: `0x68`.

## Локальное время

CoilMaster использует местное время Кыргызстана / Бишкек:

```text
IANA zone: Asia/Bishkek
UTC offset: +06:00
ESP32 POSIX TZ: KGT-6
```

## Автоматическая NTP-синхронизация

После успешного подключения ESP32 к сохранённой Wi-Fi STA сети прошивка
настраивает NTP. Серверы:

```text
pool.ntp.org
time.google.com
time.cloudflare.com
```

После получения валидного сетевого времени ESP32 переводит его в локальное
время Бишкек и записывает в DS3231.

Правила:

- без интернета DS3231 продолжает работать автономно;
- после появления/возврата интернета синхронизация запускается автоматически;
- при временной ошибке NTP выполняются повторные попытки каждые 30 секунд;
- после успешной синхронизации DS3231 обновляется примерно каждые 6 часов;
- запись откладывается, пока CoilMaster не находится в безопасном idle-состоянии;
- синхронизация не запускает двигатель, не управляет SSR и не меняет winding job state.

## Ручная установка времени

API:

```text
POST /api/system/time
```

Поля:

```text
year
month
day
hour
minute
second
confirmed=true
```

Поля времени теперь корректно принимают ведущие нули (`08`, `05`, `00`).
Ранее общий parser идентификаторов запрещал ведущие нули и мог вернуть
`invalid_rtc_datetime_fields` для нормальной даты/времени из браузера.
Строгий canonical parser для job/session/etc. не изменён.

Ручная запись по-прежнему разрешается только при безопасном idle-состоянии.

## Диагностика

```text
GET /api/system/time
```

Ответ содержит:

- `detected`;
- `time_valid`;
- `local_time`;
- `timezone = Asia/Bishkek`;
- `utc_offset_minutes = 360`;
- `sta_connected`;
- `ntp_synchronized`;
- `ntp_status`;
- `last_ntp_sync_uptime_ms`.

Основные `ntp_status`:

```text
NOT_CONNECTED
WAITING_NTP
NTP_UNAVAILABLE
NTP_TIME_INVALID
WAITING_SAFE_IDLE
RTC_NOT_DETECTED
RTC_WRITE_VERIFY_FAILED
SYNCED
```

В Serial ESP32 после успешной синхронизации появляется:

```text
NTP: DS3231 synchronized to Bishkek YYYY-MM-DD HH:MM:SS
```
''',
    encoding="utf-8",
)

index_path = Path("docs/HARDWARE_REFERENCE/00_READ_FIRST.md")
index = index_path.read_text(encoding="utf-8")
anchor = "- `03_KEYS_AND_HIDDEN_COMMANDS.md` — клавиши 4×4, START и скрытые/аварийные команды.\n"
if anchor not in index:
    raise SystemExit("hardware reference index anchor not found")
index = index.replace(
    anchor,
    anchor
    + "- `04_RTC_TIME_SYNC.md` — DS3231, ручная установка и автоматическая NTP-синхронизация времени Бишкек.\n",
    1,
)
index_path.write_text(index, encoding="utf-8")
