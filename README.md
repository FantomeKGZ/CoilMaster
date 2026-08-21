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
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/
```

Старые numbered checkpoints в `docs/PROJECT_HANDOFF/` и capitalized `Docs/`
сохраняются как историческое evidence. Их старые `next`/`pending` разделы не
являются текущей очередью разработки.

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

`Shared/Protocol/` — ранний binary host-test protocol, не production wire layer.

JOB cancel/recovery уже реализован: safe no-run cancellation, idempotent
`ALREADY_CLEAR`, physical `D -> * -> # -> D` fallback with `ALL_CLEAR`, без
remote/automatic START и без synthetic `RUN_COMPLETED`.

## Material/writeoff semantics

Для нового KG_FIRST consumption authoritative quantity — `quantity_kg`.
Exact `source_session_id + source_run_id` provenance обязательны. `spool_id`
может отсутствовать только в утверждённом unallocated/manual KG_FIRST path;
если spool используется, его exact provenance сохраняется.

## Сборка

```text
pio run -e uno
pio run -e esp32
```

Текущий подтверждённый production baseline:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Arduino Uno Build current HEAD и current-head hardware acceptance являются
отдельными verification gates.

## Актуальная структура

```text
Arduino/                    Arduino hardware adapters + CMP1 transport
Core/                       realtime/domain winding model
firmware/arduino/src/       production Arduino entrypoint
firmware/esp32/src/         production ESP32 firmware
firmware/esp32/web/         desktop/mobile/shared web assets
Shared/CMP1Text/            production shared CMP1 CRC
Shared/Protocol/            legacy binary host-test protocol
Tests/                      protocol + web/safety regression tests
docs/AI_AGENT/              AI maintenance/navigation documentation
docs/PROJECT_HANDOFF/       current state + historical checkpoints
docs/HARDWARE_REFERENCE/    hardware operator references
Docs/                       legacy foundation documentation
```
