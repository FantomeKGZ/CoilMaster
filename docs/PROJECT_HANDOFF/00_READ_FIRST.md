# CoilMaster — current project entrypoint

Дата обновления: **2026-08-25**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: **`cmp-protocol-v1`**. `main` для исходников не использовать.

## Что читать новому AI / coding agent

```text
/AGENTS.md
this file
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/89_REPAIR_PRICING_REFERENCE_BATCHING_2026-08-25.md
docs/PROJECT_HANDOFF/88_MATERIAL_REFERENCE_BATCHING_2026-08-25.md
docs/PROJECT_HANDOFF/87_FINALIZATION_COSTING_SINGLE_PASS_2026-08-25.md
docs/PROJECT_HANDOFF/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

`90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` — текущий authoritative transfer checkpoint. Если старые numbered checkpoints противоречат `90` по текущему состоянию, verified GREEN baseline, active Stage-1 direction или следующему hardware gate, использовать `90`.

Старые numbered checkpoints — history/evidence, а не backlog. Не продолжать старую задачу только потому, что исторический checkpoint содержит `next`/`pending`.

Перед изменением/deletion existing file обязательно fetch актуального содержимого из `cmp-protocol-v1` и current blob SHA. Для нового файла сначала проверить exact path. Не утверждать CI/build/hardware GREEN без фактического результата или явного подтверждения оператора. Empty GitHub code-search не является достаточным доказательством отсутствия dependency.

## Current completion estimate

```text
Overall project readiness                         ~95%
Software/repo implementation + integrity          ~98-99%
Reference Web/site layer                          ~98%
Full two-board hardware acceptance                still required
```

Основной production flow, persistence, Web, backup, Hall split, Uno runtime, safety contracts и Stage-1 storage/performance hardening уже собраны. Оставшаяся работа — узкий repo-only performance review только по доказанным hotspots и один финальный hardware E2E после завершения software batch.

## Current verified GREEN baseline

Latest verified Stage-1 block: **89_REPAIR_PRICING_REFERENCE_BATCHING**.

```text
29ecbb799a14da455aa5d732764613465b21788a
perf(esp32): batch repair pricing references

ESP32 Build #1441
run 32818211915
head_sha 29ecbb799a14da455aa5d732764613465b21788a
SUCCESS

CMP Protocol Tests #3096
run 32818305639
head_sha 921999a8f2a11405c8a312a4f6064c2a29834e93
SUCCESS
```

Final CMP run includes GREEN repair pricing batching, material batching, scoped backup, final acceptance, KG_FIRST, winding/finalization/workshop single-pass, write-off fault, NDJSON growth diagnostics and Hall contract audits.

Hardware GREEN из CI не выводить.

## Current active phase

Software cleanup и основной production implementation закрыты. Текущая работа — **Stage-1 ESP32/storage performance review** перед финальным two-board hardware acceptance.

Искать только:

- per-record full-file reference scans;
- доказанные duplicate authoritative scans с эквивалентной семантикой;
- явно измеримые avoidable allocations/I/O без ослабления fail-closed integrity.

Если повторный scan обеспечивает отдельную семантику и его нельзя убрать без unbounded RAM/indexing — помечать `KEEP` и переходить дальше.

Current KEEP:

- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy ESP32 `CAL_RESULT` receive fallback.

No premature database migration. No automatic NDJSON cleanup/rotation. Thresholds only from measured real-device data.

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
- linked-production manual writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
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

## Uno resource baseline

Latest verified baseline:

```text
RAM   1205 / 2048 = 58.8%   free 843 B
Flash 31460 / 32256 = 97.5%  free 796 B
```

Flash is limiting. CI guard requires >=512 B free RAM and >=512 B free flash.

## External hardware gate

По решению пользователя промежуточные hardware tests во время software optimization phase не запрашивать.

После исчерпания оправданных Stage-1 candidates выполнить один полный hardware acceptance ESP32 + Arduino Uno: UART/JOB, physical START, RUN events, repeat, cancel/recovery, reboot fail-closed, Hall calibration/apply/reconciliation, keypad/LCD/buzzer и Uno-only SSR ownership.

Точная current state, completion estimate, recent commits, KEEP decisions и ready-to-paste prompt для нового чата находятся в:

```text
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```
