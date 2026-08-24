# CoilMaster — current project entrypoint

Дата обновления: **2026-08-24**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/73_NEXT_CHAT_TRANSFER_2026-08-24.md
docs/PROJECT_HANDOFF/72_HALL_COMPACT_COMPLETION_ACTIVE_2026-08-24.md
docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md
docs/PROJECT_HANDOFF/70_HALL_CALIBRATION_HISTORY_2026-08-24.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

`73_NEXT_CHAT_TRANSFER_2026-08-24.md` — текущий authoritative transfer checkpoint. Если старые numbered checkpoints противоречат `73` по текущему Hall/Uno состоянию или следующему шагу, использовать `73`.

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением/deletion existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора. Empty GitHub code-search не является достаточным доказательством отсутствия dependency.

## Current verification baseline

Текущий Hall compact-completion software checkpoint подтверждён свежими GitHub Actions:

```text
32753340348  host-tests  checkout d77a24b3437831d0a236086055a193f233e1be7e  SUCCESS
32753408620  host-tests  checkout b07de01ee4f3b1216153036dd977fa48bc053c2f  SUCCESS
```

Последний run прошёл Hall calibration safety, lost-apply reconciliation, Uno parser ownership, Hall raw migration, Hall history и остальные host gates.

Verified Uno compact-completion build:

```text
32751199627  build-uno  checkout a928a51bc77c00407b146587aaf34c1e08a19998  SUCCESS
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31640 / 32256 = 98.1% free 616 B
```

Hardware GREEN из CI не выводить.

## Current active phase

Software cleanup закрыт. Текущая работа — финальная software optimization минимального Arduino Uno runtime и Hall calibration split перед единственным полным hardware acceptance.

Hall extended aggregation уже перенесена на ESP32; Uno сохраняет physical START, SSR authority, обычный realtime Hall turn count и local safety gates. Uno completion TX уже использует compact `CAL_DONE`; ESP32 сохраняет legacy `CAL_RESULT` receive fallback.

Точный current state и следующий шаг: `docs/PROJECT_HANDOFF/73_NEXT_CHAT_TRANSFER_2026-08-24.md`.

Не начинать broad cleanup заново без конкретного нового inconsistency, failing test, runtime defect или stale contract evidence.

## Safety invariants

Never weaken:

- physical START only physical/local;
- no automatic physical START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly control SSR;
- lost ACK / timeout never proves Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` never performs automatic wire/material writeoff;
- current linked-production manual writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- historical `UNALLOCATED` KG_FIRST remains read/audit/recovery compatibility evidence only, not permission to drop a selected spool from a new run;
- operational cancellation does not erase immutable run/history evidence;
- backup restore operator-only, transactional and fail-closed;
- reboot never auto-continues restore/apply;
- no automatic production-data deletion/truncation.

## Production architecture

```text
ESP32: service/data/UI orchestration, SD/RTC/network, registry, jobs,
       persistence, warehouse/material/costing, backup/restore,
       extended Hall calibration analysis/history

CMP1 UART: JOB/control down, run/status/calibration events up

Arduino Uno: physical START, SSR, normal Hall turn count,
             keypad/LCD/buzzer, calibration local safety gates,
             realtime winding machine and RUN event generation
```

Production flow:

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable snapshot + exact immutable spool selection -> UART JOB
-> physical START -> RUN_STARTED/RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## External hardware gate

По решению пользователя промежуточные hardware tests во время текущей optimization phase не запрашивать. Использовать software gates/CI/size/contracts.

После завершения оптимизации выполнить один полный hardware acceptance из `73_NEXT_CHAT_TRANSFER_2026-08-24.md`.

Hardware GREEN никогда не выводить из CI.

## Next work rule

Продолжать с `73_NEXT_CHAT_TRANSFER_2026-08-24.md`. Первый рекомендованный шаг — доказать точную максимальную длину оставшихся Uno Hall response frames и только после этого решать, можно ли безопасно уменьшить `HallCalibrationProtocol::MaxFrameLength = 96`.
