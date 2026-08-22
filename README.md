# CoilMaster

CoilMaster — локальная система управления намоточным станком, учёта ремонтов,
двигателей, материалов и резервных копий.

Единственная рабочая/source-of-truth ветка исходников: `cmp-protocol-v1`.
Ветка `main` не используется как источник реализации.

## С чего начинать

Для человека или AI/coding agent:

```text
AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/
```

Старые numbered checkpoints в `docs/PROJECT_HANDOFF/` являются историческим
evidence, а не активной очередью разработки.

## Текущая фаза

Полный repo-level аудит A–E завершён. После подтверждённого GREEN начата
контролируемая cleanup/de-duplication phase: файл удаляется только после
проверки production/build/test/docs/runtime зависимостей.

Последний подтверждённый пользователем GREEN baseline перед очередным cleanup
набором:

```text
51ea46c1823a451e7f80ecd188daf896aafc752d
Fix production conductor cleanup contract
USER CONFIRMED GREEN
```

Commits после этого SHA не считаются GREEN автоматически.

## Контроллеры

- Arduino Uno выполняет realtime-логику намотки, считает Hall, принимает
  физический START и владеет SSR.
- ESP32 обслуживает Wi-Fi, HTTP/FTP, microSD, RTC, workshop registry,
  warehouse/materials/costing, backup/restore и доставку JOB Arduino.

ESP32/Web не управляют SSR напрямую. После reboot нет automatic resume.
`RUN_COMPLETED` не списывает материал автоматически.

## Production UART

Рабочий Arduino <-> ESP32 протокол — текстовый `CMP1|...`:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` — binary host-test protocol, не production wire layer. Его
назначение и формат теперь документированы рядом с кодом в
`Shared/Protocol/README.md`.

## Material/writeoff semantics

Для KG_FIRST consumption authoritative quantity — `quantity_kg`.
Exact `source_session_id + source_run_id` provenance обязательны. `spool_id`
может отсутствовать только в утверждённом unallocated/manual KG_FIRST path;
если spool используется, его exact provenance сохраняется.

## Сборка

```text
pio run -e uno
pio run -e esp32
```

Hardware acceptance остаётся отдельным verification gate и не выводится из CI.

## Актуальная структура

```text
Arduino/                    Arduino hardware adapters + CMP1 transport
Core/                       realtime/domain winding model
firmware/arduino/src/       production Arduino entrypoint
firmware/esp32/src/         production ESP32 firmware
firmware/esp32/web/         desktop/mobile/shared web assets
Shared/CMP1Text/            production shared CMP1 CRC
Shared/Protocol/            binary host-test protocol + local README
Tests/                      protocol + web/safety regression tests
docs/AI_AGENT/              AI maintenance/navigation documentation
docs/PROJECT_HANDOFF/       current state + historical checkpoints
docs/HARDWARE_REFERENCE/    hardware operator references
```
