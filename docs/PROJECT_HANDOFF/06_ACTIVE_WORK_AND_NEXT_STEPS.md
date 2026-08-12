# Где остановились и что делать дальше

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Hardware production status

На реальном стенде подтверждены:

```text
Standalone Arduino local program
→ physical START
→ real winding
→ LOCAL_EVT
→ ESP32 autonomous archive
```

и полный linked production flow:

```text
client / motor / OPEN repair
→ linked winding
→ exact spool
→ UART
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing / finalization
→ CLOSED
→ final data found
→ backup/manifest readable
```

Основной happy-path hardware E2E закрыт.

Hardware negative checkpoint также подтверждён:

```text
active RUN_STARTED
→ backup request
→ export blocked
→ deep stability scan not started
```

## Safety boundary — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и exact `spool_id + source_session_id + source_run_id`.

Arduino pin-map:

```text
D11 = Buzzer
D12 = SSR
A0  = Hall
A1  = Arduino TX → ESP32 RX
A2  = Arduino RX ← ESP32 TX
```

## Последний подтверждённый build

Последний пользовательский clean ESP32 build до текущих paging/recovery изменений:

```text
RAM:   14.4% (47320 / 327680 bytes)
Flash: 86.7% (1136237 / 1310720 bytes)
SUCCESS
```

Текущий HEAD изменён после этого build, поэтому сейчас:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

GitHub CI не считать green без фактического result.

## Backup / integrity hardware baseline

Реальный manifest ранее подтвердил:

```text
export_allowed=true
activity_state_verified=true
snapshot_stability_checked=true
snapshot_stable=true
snapshot_stability_reason=null
snapshot_stability_duration_ms=1429
```

Baseline:

```text
docs/PROJECT_HANDOFF/14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
```

Production E2E:

```text
docs/PROJECT_HANDOFF/15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
```

## Закрытый fault-hardening

### Backup / reboot

`MANUAL_REVIEW_REQUIRED` и persisted ambiguous states fail-closed блокируют backup.

### Runtime storage corruption

Allocator/state paths заново проверяются перед новой session; `.tmp`, повреждённые state/high-water и неоднозначный recovery блокируют job creation.

### UART timeout / replay

`JOB_ACK` timeout больше не считается безопасным terminal state:

```text
TIMED_OUT
→ MANUAL_REVIEW_REQUIRED
→ new job blocked
→ backup blocked
→ no auto resend / auto resume
```

Linked duplicate требует exact event semantics. Autonomous duplicate требует exact:

```text
session_id
run_id
event_type
completed_runs
job_type
coil_count
program
```

### Controlled recovery

Оба recovery endpoint теперь перед `CLOSED_AFTER_REVIEW` перепроверяют persisted latest state + immutable snapshot; linked job дополнительно требует exact immutable spool-selection. Recovery closure выполняет ESP32 restart.

Backup activity guard также повторно доказывает persisted state/snapshot/spool identity даже при runtime `Safe`, поэтому повреждённый linked recovery не может использовать file-export как обход deep integrity.

Подробнее:

```text
docs/PROJECT_HANDOFF/17_RUNTIME_STORAGE_CORRUPTION_HARDENING_2026-08-12.md
docs/PROJECT_HANDOFF/18_UART_TIMEOUT_REPLAY_HARDENING_2026-08-12.md
docs/PROJECT_HANDOFF/19_CONTROLLED_RECOVERY_AND_ARCHIVE_SCALING_2026-08-12.md
```

## Autonomous Arduino archive — current scaling state

Authoritative storage остаётся:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Формат не мигрирован в БД и не ротируется автоматически.

Deep backup audit уже переведён с repeated per-record scans на:

```text
events       O(E)
assignments  fixed batches of 32 references
```

Теперь также закрыт unbounded HTTP/UI growth.

`GET /api/autonomous-windings`:

```text
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
```

Cursor выдаётся только на logical task boundary и не разрезает normal `RUN_STARTED/RUN_COMPLETED` pair.

Mobile/Desktop UI:

```text
20 tasks per normal page
→ Показать ещё
→ next_cursor
```

Полный проход для раздела «Скомплектованные двигатели» больше не происходит автоматически при каждом поиске; он запускается отдельной кнопкой и также читается pages по 32.

Checkpoint:

```text
docs/PROJECT_HANDOFF/20_AUTONOMOUS_ARCHIVE_PAGING_2026-08-12.md
```

## Важный UI deployment note

Paging меняет HTML assets:

```text
firmware/esp32/web/mobile/arduino-windings.html
firmware/esp32/web/desktop/arduino-windings.html
```

После успешного firmware build/upload заменить microSD `/web` актуальным содержимым repo `firmware/esp32/web`.

Предпочтительно полное replacement `/web`, а не overlay поверх старых файлов.

## Следующее действие

Сначала clean build текущего HEAD:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

После `SUCCESS`:

```powershell
pio run -e esp32 -t upload
```

и обновить microSD `/web`.

Минимальная runtime paging проверка:

```text
GET /api/autonomous-windings?limit=1
```

Если архив содержит больше одной logical task:

```text
count=1
has_more=true
next_cursor=<number>
```

Затем:

```text
GET /api/autonomous-windings?limit=1&cursor=<next_cursor>
```

должен вернуть следующую logical task без повторения и без разделения START/COMPLETE pair.

## Следующий repo-reviewable performance блок после build

1. Перевести boot-time `AutonomousWindingArchive::begin()` со старых repeated validators на authoritative bounded-complexity `validateStorage()`.
2. Снять populated-dataset timings через `/api/backup/manifest`.
3. Оценить NDJSON rotation/segmentation только по реальным размерам и latency.
4. Не вводить DB migration или destructive compaction без доказанной необходимости.

Основная новая функциональность сейчас не приоритет; проект находится в production-hardening/performance phase.
