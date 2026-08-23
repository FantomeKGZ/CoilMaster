# CoilMaster — концепция проекта

## Назначение

CoilMaster — локальная модульная система управления намоточным станком и учёта работ по перемотке электродвигателей.

Система состоит из двух контроллеров с жёстко разделённой ответственностью:

- **Arduino Uno** — realtime-контроллер физического процесса: Hall, SSR, keypad, physical START, LCD1602, buzzer, локальная state machine и фактические `RUN_STARTED` / `RUN_COMPLETED`;
- **ESP32** — service/data/UI-контроллер: Wi‑Fi/AP, HTTP, microSD, RTC DS3231, workshop registry, winding-job persistence, warehouse/materials/costing, backup/restore и CMP1 UART orchestration.

Production source-of-truth branch: `cmp-protocol-v1`.

## Основные цели

1. Надёжно выполнять намотку катушек с разным количеством витков.
2. Сохранять локальный physical START и SSR authority только на Arduino.
3. Передавать подготовленные задания между Web/ESP32 и Arduino без удалённого физического запуска.
4. Сохранять фактические run events с точными `session_id + run_id`.
5. Хранить клиентов, общие карточки двигателей, ремонты, материалы, costing и историю операций.
6. Поддерживать desktop/mobile Web UI и отдельные read-only reference datasets/sites на microSD.
7. Предоставлять безопасную Hall calibration/telemetry, диагностику, backup и recovery.

## Принципы архитектуры

- Arduino является источником истины о физическом процессе намотки и единственным владельцем SSR.
- ESP32 является источником истины о persisted business/service state и Web/API orchestration.
- Web/ESP32 не включают SSR напрямую и не создают physical START.
- `JOB_ACK ACCEPTED` означает только принятие задания; запуск остаётся физическим.
- После reboot нет automatic winding resume.
- Потеря ACK/timeout не доказывает, что Arduino idle.
- `RUN_COMPLETED` никогда сам по себе не списывает провод.
- Для текущего linked-production manual wire writeoff обязательны exact `source_session_id + source_run_id + immutable spool_id`.
- Historical `UNALLOCATED` KG_FIRST records остаются compatibility/read/audit evidence и не разрешают новому linked job потерять выбранный spool.
- Production persistence и restore работают fail-closed; production evidence не удаляется автоматически при нехватке места.
- Arduino Uno implementation ориентирован на статические/ограниченные структуры и контролируемый Flash/SRAM budget; новые зависимости оцениваются по фактической стоимости ресурсов, а не по абстрактному запрету отдельного C++ типа.

## Основные сущности и связи

Карточка двигателя не принадлежит одному клиенту. Клиент и двигатель связываются через конкретный ремонт:

```text
Client ----\
            -> OPEN Repair -> linked winding job/session/run
Motor -----/                 -> exact spool/material usage
                              -> costing/finalization
                              -> CLOSED
```

Отдельно сохраняются immutable winding snapshot/spool-selection evidence, фактические run events, material movements/cost snapshots и backup/recovery evidence.

## Production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> exact immutable spool selection + immutable job snapshot/state
-> UART JOB -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization preflight -> CLOSED -> reports -> backup
```

## Protocol boundary

Production Arduino ↔ ESP32 wire protocol — текстовый `CMP1|...` с CRC и bounded parsing:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` — отдельный старый binary CMP, используемый host tests; это не production wire layer.

## Status source

Этот документ фиксирует устойчивую концепцию. Текущий процент готовности, verified GREEN baseline и активная очередь не дублируются здесь и берутся из:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```
