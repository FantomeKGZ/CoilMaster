# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

Этот checkpoint является текущим authoritative transfer для продолжения проекта в новом чате. Старые numbered checkpoints остаются history/evidence и не являются активным backlog.

## Completion estimate

Текущая оценка общей готовности проекта: **~95%**.

Разделение:

```text
Core software architecture / production flow      ~99%
ESP32 services, Web, persistence, backup          ~98%
Arduino Uno runtime / CMP1 / Hall split            ~97%
Integrity, recovery, CI regression coverage        ~99%
Reference site / SD web bundle                     ~98%
Full two-board hardware acceptance                 pending final E2E
Overall release readiness                          ~95%
```

Почему не 100%:

1. После переноса расширенного Hall анализа на ESP32 и оптимизации Uno всё ещё требуется один полный hardware E2E на реальных ESP32 + Arduino Uno.
2. Нужно проверить реальный production flow на двух платах: UART delivery, physical START, RUN_STARTED/RUN_COMPLETED, repeat behavior, cancel/recovery, Hall calibration apply/reconciliation, keypad/LCD/SSR ownership.
3. NDJSON rotation/threshold policy нельзя финализировать без реальных device metrics; автоматическая cleanup/rotation пока намеренно не включается.
4. Оставшиеся repo-only performance изменения делать только при доказанном duplicate scan / O(n*m) hotspot, не ради абстрактной оптимизации.

## Source of truth / working rules

- Единственная source-of-truth ветка: **`cmp-protocol-v1`**.
- `main` не использовать как источник кода.
- Перед изменением existing file: fetch exact current content из `cmp-protocol-v1` + current blob SHA.
- Перед созданием new file: проверить exact path и убедиться в 404/not found.
- Не объявлять CI/build GREEN без фактической проверки.
- Не просить промежуточные hardware tests во время software batch; hardware acceptance выполняется один раз после завершения software optimization batch.
- После meaningful change обновлять `docs/PROJECT_HANDOFF`.

## Safety invariants — never weaken

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drives SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- writeoff stays explicit/manual;
- linked-production writeoff is tied to exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion or NDJSON truncation.

## Production ownership

```text
ESP32:
  Wi-Fi/AP/HTTP/FTP
  SD/RTC
  workshop registry
  jobs/persistence
  warehouse/material/costing
  backup/restore
  Web UI
  extended Hall calibration analysis/history

CMP1 UART:
  commands/jobs/config down
  run/status/calibration events up

Arduino Uno:
  physical START
  SSR authority
  normal Hall realtime turn count
  keypad/LCD/buzzer
  local calibration confirmation/safety gates
  winding realtime state machine
  RUN_STARTED/RUN_COMPLETED generation
```

Production wire protocol remains text `CMP1|...`. `Shared/Protocol/` is older host/test protocol code and is not the production replacement.

## Production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> exact immutable spool selection + immutable snapshot
-> UART JOB -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> explicit manual exact-run exact-spool writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

## Hall architecture

Uno owns realtime Hall threshold/hysteresis/debounce/direction and local physical safety.
ESP32 owns raw sample aggregation, baseline/min/max/span/count/duration, recommendation, history, Web/status and proposal orchestration.

Calibration flow:

```text
ESP32 CAL_ARM
-> Uno WAITING_LOCAL_CONFIRM
-> local #
-> ARMED_WAITING_START
-> baseline
-> separate physical START
-> RUNNING
-> raw CAL_SAMPLE stream + CAL_DONE measurement_id
-> ESP32 analysis/recommendation
-> CAL_PROPOSAL exact measurement_id
-> Uno WAITING_APPLY_CONFIRM
-> local #
-> EEPROM apply
```

ESP32 intentionally keeps legacy `CAL_RESULT` receive fallback. Uno no longer emits it.
Lost `CAL_APPLIED` does not replay proposal; CFG_GET may reconcile settings but equality does not prove the exact apply event.

## Uno resource state

Latest verified production size baseline after optimization:

```text
RAM   1205 / 2048 = 58.8%   free 843 B
Flash 31460 / 32256 = 97.5%  free 796 B
```

Flash is the limiting resource. CI guard requires at least 512 B free RAM and at least 512 B free flash.
Do not reintroduce code-heavy SRAM micro-optimizations that increase flash.

## Recent Stage-1 storage/performance blocks — GREEN

Completed and CI-verified:

```text
80  writeoff single-pass movement lookup
81  winding completion single-pass
82  finalization winding single-pass
83  material backup scoped audit
84  warehouse backup scoped audit
85  NDJSON performance/rotation strategy (no premature DB migration)
86  workshop winding single-pass
87  finalization costing/movement single-pass
88  material reference batching (batch=32)
89  repair pricing reference batching (batch=32)
```

Latest block 89 commits:

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

Final CMP includes GREEN:

- material reference batching;
- repair pricing reference batching;
- material/warehouse backup scoped contracts;
- final acceptance;
- kg-first material contracts;
- winding completion/finalization/persistence/workshop single-pass;
- write-off fault contracts;
- NDJSON growth diagnostics;
- all Hall safety/telemetry/apply/ownership/history audits.

## KEEP decisions from latest review

Do not optimize these without new evidence:

- `repair-status` bounded self-scan: CLOSED records can arrive in arbitrary repair-id order; removing the bounded re-scan would require unbounded RAM/indexing.
- autonomous assignment reference batching: assignments may target old completed runs in arbitrary operator order, therefore a bounded event re-scan is semantically required.
- warehouse movement provenance uniqueness batching.
- Uno `writeJobReply()` stack buffer, LCD hashes, keypad maps/pins, millis uint32, legacy CAL_RESULT fallback.

## NDJSON strategy

No premature database migration.
No automatic cleanup/rotation yet.
`/api/system/storage` provides growth observability.
Threshold/rotation policy must be based on measured real-device data.
Repo-only optimization should focus on proven duplicate authoritative scans and per-record full-file reference lookups while preserving exact uniqueness and fail-closed behavior.

## Reference Web/site status

Legacy winding reference integration is effectively software-complete and physical microSD/ESP32 smoke was previously reported without errors.
Current estimate for reference/site layer remains ~98%.
Generated output includes desktop/mobile trees, shared assets, integrity/fidelity checks and SD bundle provenance.
The two-board UART hardware gate is separate from the reference-site smoke.

## Next repo-only direction

Continue a narrow Stage-1 storage/performance audit only when a real hotspot is found.
Priority classes:

1. per-record full-file reference scans still present in ESP32 persistence/audit code;
2. adjacent authoritative validators scanning the same NDJSON twice with equivalent semantics;
3. unbounded or repeated `String`/JSON allocation only when measurable and semantically safe to remove;
4. otherwise mark KEEP and move on.

Do not redesign working batching just because multiple passes exist when those passes enforce distinct semantics.

## Final hardware acceptance — still required

After software optimization stops producing justified candidates, perform one complete hardware acceptance on ESP32 + Arduino Uno.
Minimum acceptance:

```text
boot both boards
UART CMP1 handshake/status
send one linked JOB from ESP32
confirm Uno receives it but does not auto-start
physical START only
RUN_STARTED observed on ESP32
Hall turn counting stable
RUN_COMPLETED observed on ESP32
no automatic wire writeoff
manual exact-run exact-spool writeoff
repeat requires another physical START
cancel/recovery path
reboot: no auto-resume/no physical start
Hall calibration full ARM -> local confirm -> physical start -> CAL_DONE -> proposal -> local apply
lost-apply reconciliation / CFG state sanity
keypad/LCD/buzzer usable
SSR controlled only by Uno
```

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/89_REPAIR_PRICING_REFERENCE_BATCHING_2026-08-25.md
docs/PROJECT_HANDOFF/88_MATERIAL_REFERENCE_BATCHING_2026-08-25.md
docs/PROJECT_HANDOFF/87_FINALIZATION_COSTING_SINGLE_PASS_2026-08-25.md
docs/PROJECT_HANDOFF/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

## Ready-to-paste prompt for the new chat

```text
Продолжаем проект CoilMaster.

Репозиторий: FantomeKGZ/CoilMaster.
Единственная source-of-truth ветка: cmp-protocol-v1. main для исходников не использовать.

Сначала ознакомься с:
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
и указанным там read order.

Продолжай сразу по коду и коммитам, без долгого планирования. Перед каждым изменением existing file сначала fetch его актуального содержимого именно из cmp-protocol-v1 и используй текущий blob SHA. Перед созданием нового файла сначала проверь exact path. Не утверждай, что CI/build GREEN, пока это фактически не проверено.

Safety-инварианты не менять:
- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- lost ACK/timeout не означает, что Arduino idle;
- final repeat не может auto-reopen;
- RUN_COMPLETED сам не списывает провод;
- списание провода остаётся ручным и связано с exact source_session_id + source_run_id + immutable spool_id;
- restore operator-only и fail-closed;
- никаких automatic production-data deletion/NDJSON truncation.

Текущая общая готовность проекта около 95%; software/repo часть около 98-99%. Основной production flow уже собран.

Последний полностью GREEN блок: 89_REPAIR_PRICING_REFERENCE_BATCHING.
Финальный descendant: 921999a8f2a11405c8a312a4f6064c2a29834e93.
ESP32 Build #1441 run 32818211915 GREEN.
CMP #3096 run 32818305639 GREEN, включая новый repair pricing batching audit и все ключевые Hall/material/backup/finalization audits.

Текущая работа: узкий Stage-1 ESP32/storage performance review. Ищи только доказанные duplicate authoritative scans или per-record full-file reference lookups. Сохраняй bounded RAM, exact uniqueness и fail-closed semantics. Если scan семантически нужен — помечай KEEP и переходи дальше.

Не трогать без новых доказательств:
- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy CAL_RESULT fallback.

После исчерпания оправданных software optimizations требуется один полный двухплатный hardware acceptance ESP32+Arduino Uno. Промежуточные hardware tests пока не запрашивать.

Продолжаем с текущего состояния ветки.
```
