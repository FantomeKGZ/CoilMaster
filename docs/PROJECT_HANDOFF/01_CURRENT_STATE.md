# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает только текущее состояние. История и доказательства находятся в numbered checkpoints.

## Source of truth

Единственный источник реализации — `cmp-protocol-v1`. `main` для исходников не использовать.
Перед изменением existing file получать current content + blob SHA. Для нового path сначала проверять отсутствие файла. Не объявлять CI/build/hardware GREEN без фактической проверки.

## Completion estimate

```text
Overall project readiness                         ~95%
Software/repo implementation + integrity          ~98-99%
Reference Web/site layer                          ~98%
Final two-board hardware acceptance               pending
```

Проект находится в стадии финальной оптимизации и интеграционной приёмки, а не основной разработки.

## Current verified GREEN baseline

Latest verified Stage-1 block: `89_REPAIR_PRICING_REFERENCE_BATCHING_2026-08-25.md`.

```text
29ecbb799a14da455aa5d732764613465b21788a  perf(esp32): batch repair pricing references
e027e86fe66a9ea2473d53ed01dd69ace279e5a7  test(esp32): protect repair pricing reference batching
91ea3ee824b4589e88ec2c2cd7c063ad3e3c7ffe  ci(esp32): audit repair pricing reference batching
921999a8f2a11405c8a312a4f6064c2a29834e93  docs(handoff): record repair pricing reference batching
```

Verified Actions:

```text
ESP32 Build #1441 / run 32818211915 / head 29ecbb79... / SUCCESS
CMP #3093 / run 32818211986 / head 29ecbb79... / SUCCESS
CMP #3094 / run 32818234743 / head e027e86f... / SUCCESS
CMP #3095 / run 32818272548 / head 91ea3ee8... / SUCCESS
CMP #3096 / run 32818305639 / head 921999a8... / SUCCESS
```

Final CMP includes GREEN material/repair-pricing batching, scoped backup, final acceptance, KG_FIRST, winding/finalization/workshop single-pass, write-off fault, NDJSON growth diagnostics and Hall contract audits.

## Production architecture

```text
ESP32:
  Wi-Fi/AP/HTTP/FTP, SD/RTC, workshop registry, jobs/persistence,
  warehouse/material/costing, backup/restore, Web UI,
  extended Hall calibration analysis/history

CMP1 UART:
  JOB/control/config down
  run/status/calibration events up

Arduino Uno:
  physical START, SSR authority, normal Hall realtime count,
  keypad/LCD/buzzer, calibration local safety gates,
  realtime winding state machine, RUN_STARTED/RUN_COMPLETED
```

Production protocol is text `CMP1|...`. `Shared/Protocol/` is older host/test protocol code, not production wire protocol.

## Safety boundary

Never weaken:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never auto-writes off material;
- manual linked-production writeoff requires exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/NDJSON truncation.

## Production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable snapshot + exact immutable spool selection
-> UART JOB -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## Hall ownership

Uno keeps realtime Hall threshold/hysteresis/debounce/direction, physical START and local safety gates.
ESP32 owns raw sample aggregation, baseline/min/max/span/count/duration, recommendation, Web/status/history and proposal orchestration.

Calibration:

```text
CAL_ARM -> local # -> ARMED_WAITING_START -> baseline
-> physical START -> RUNNING -> CAL_SAMPLE/CAL_DONE
-> ESP32 analysis -> CAL_PROPOSAL exact measurement_id
-> local # -> EEPROM apply
```

ESP32 intentionally keeps legacy `CAL_RESULT` receive fallback. Uno no longer emits it.
Lost CAL_APPLIED does not cause proposal replay; CFG_GET can reconcile settings but does not prove exact apply event.

## Uno resource baseline

```text
RAM   1205 / 2048 = 58.8%   free 843 B
Flash 31460 / 32256 = 97.5%  free 796 B
```

Flash is limiting. CI guard requires >=512 B free RAM and >=512 B free flash.

## Storage/performance state

Stage-1 blocks 80-89 are implemented and GREEN, including:

- writeoff movement single-pass;
- winding completion/finalization/workshop single-pass;
- material/warehouse backup scoped audits;
- finalization costing/movement single-pass;
- material reference batching batch=32;
- repair pricing reference batching batch=32.

NDJSON strategy: no premature DB migration, no automatic cleanup/rotation, thresholds only from measured real-device data.

Current KEEP decisions:

- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy ESP32 CAL_RESULT receive fallback.

## Reference Web/site

Legacy winding-reference integration, fidelity/integrity tooling and SD bundle are effectively software-complete. Physical microSD/ESP32 reference smoke was previously reported without errors. The separate ESP32<->Arduino two-board gate is still pending.

## Current active direction

Continue only narrow Stage-1 repo-only performance review:

1. proven per-record full-file reference scans;
2. proven duplicate authoritative scans with equivalent semantics;
3. measurable avoidable allocation/I/O without weakening fail-closed integrity;
4. otherwise mark KEEP and move on.

After justified repo-only candidates are exhausted, perform one full two-board hardware acceptance.

Authoritative next-chat transfer:

```text
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```
