# Hall calibration + RU LCD acceptance record

Дата: **2026-08-30**  
Ветка: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — не изменён.

## Purpose

Этот файл фиксирует repo-reviewable acceptance состояние Hall calibration + русского LCD после checkpoint 166 и последующий physical two-board E2E на реальном CoilMaster.

## Reachable Hall LCD states

На Arduino LCD допускаются только три operator-visible Hall состояния:

1. `ArmedWaitingPhysicalStart`
   - строка 1: `ДАТЧИК ХОЛЛА`
   - строка 2: `A ИЛИ START`
   - запуск только локальный: keypad `A` или отдельная физическая START.
2. `Running`
   - строка 1: `ТЕСТ ХОЛЛА`
   - строка 2: `ОСТ. <n> СЕК`
   - фиксированная длительность калибровки остаётся 15 секунд.
3. `WaitingApplyConfirm`
   - строка 1: `СОХР. НАСТР.?`
   - строка 2: `#=ДА B=НЕТ`
   - применение результата остаётся явным действием оператора.

`WaitingLocalConfirm` не является достижимым LCD-экраном и не должен возвращаться в UI. ARM compatibility state внутри Hall service не даёт ESP32/Web права запускать мотор.

## RU glyph budget

Hall UI использует отдельный bounded CGRAM набор из четырёх уже существующих glyph bitmap:

```text
slot 1 = Д
slot 2 = Ч
slot 3 = И
slot 4 = Л
```

Новые bitmap не добавлены.

После выхода из Hall mode `Lcd1602View` обязан перезагрузить screen-specific RU glyph set. Это обязательный контракт: одинаковые номера CGRAM slot в обычных экранах имеют другое значение, поэтому сохранение Hall glyph cache после выхода привело бы к неверным русским буквам.

## Safety invariants

Checkpoint 166 не меняет:

- physical START ownership;
- Arduino SSR ownership;
- ESP32/Web не управляет SSR;
- no automatic physical START;
- no auto-resume after reboot;
- Hall motor permit существует только во время `Running`;
- fail-closed SSR off path;
- Hall fixed-duration timing;
- ADC sampling cadence;
- decimated UART telemetry;
- baseline interlock;
- apply/reject persistence flow.

## Implementation / contracts

```text
1162bd798b30494b9a04436ea0cd94571e8b6833  reachable Hall LCD localized in RU build
15f627c6971520f6dec9ed031e79917cce15cf7e  LCD/CGRAM safety contract
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c  RU Hall experiment contract aligned; verified source HEAD
```

Intermediate diagnostic:

```text
Arduino RU LCD #205
run 33268835043 / FAILURE
```

Причина: stale source-text contract всё ещё ожидал английский `HALL TEST READY`; failure произошёл до PlatformIO compile. Firmware regression этим run не подтверждался.

Final verified source evidence:

```text
3624e18a4c1a51fe1b914b5aa7fc3ece6245197c
CMP Protocol Tests #4028  run 33268897356 / SUCCESS
Arduino RU LCD #206       run 33268897370 / SUCCESS
```

## Uno resource gate

Exact #206 build sizes:

```text
uno_ru_lcd
RAM   1614 / 2048  = 78.8%
Flash 31448 / 32256 = 97.5%
Flash headroom = 808 bytes

uno fallback
RAM   1605 / 2048  = 78.4%
Flash 31066 / 32256 = 96.3%
Flash headroom = 1190 bytes
```

Практическое следствие: Arduino Uno больше не является подходящим местом для широкого расширения Hall processing/UI. Новые Uno-side изменения допустимы только при конкретном дефекте и должны быть очень малы. Расширенную обработку/представление по возможности держать на ESP32, сохраняя автономную безопасность Arduino.

## Physical two-board E2E — operator-confirmed PASS

На **2026-08-30** пользователь сообщил, что физический тест CoilMaster с Arduino + ESP32 проведён и **всё работает нормально**.

Это является operator-confirmed hardware evidence, а не автоматически наблюдаемым CI результатом. На основании отчёта пользователя физический acceptance gate считается пройденным для текущего проверенного hardware/firmware состояния.

Acceptance scope, для которого этот gate предназначен:

- boot без reset loop;
- keypad остаётся responsive;
- normal RU screens корректны до и после Hall mode;
- Hall armed state не запускает мотор автоматически;
- local keypad/physical START ownership сохраняется;
- SSR остаётся Arduino-owned и fail-safe;
- 15-second Hall run работает;
- apply/reject paths работают;
- ordinary RU glyphs восстанавливаются после Hall CGRAM;
- ESP32 loss не создаёт automatic resume/start.

Если позже конкретный пункт из этого списка даст regression на железе, считать это новым hardware finding и исправлять минимально, не переоткрывая остальные закрытые checkpoints.

## Acceptance status

Repo-reviewable checkpoint 166 закрыт, а physical Arduino+ESP32 Hall/RU-LCD E2E теперь также подтверждён пользователем.

Hall/RU-LCD acceptance для текущего experiment состояния считается **COMPLETE**. Новые изменения в этой области требуются только при конкретном дефекте, regression или новом product requirement.
