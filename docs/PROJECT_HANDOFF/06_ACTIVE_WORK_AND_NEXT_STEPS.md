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
→ stable backup readable
```

Happy-path hardware E2E закрыт.

Hardware negative checkpoint подтверждён:

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

## Verification status текущего HEAD

Последний подтверждённый пользователем ESP32 build был до текущих archive/workshop scaling изменений:

```text
RAM:   14.4% (47320 / 327680 bytes)
Flash: 86.7% (1136237 / 1310720 bytes)
SUCCESS
```

Текущий HEAD изменён после него:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

GitHub CI не считать green без фактического result.

## Закрытый production-hardening

Закрыты и задокументированы:

```text
14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
16_HARDWARE_NEGATIVE_BACKUP_ACTIVE_2026-08-12.md
17_RUNTIME_STORAGE_CORRUPTION_HARDENING_2026-08-12.md
18_UART_TIMEOUT_REPLAY_HARDENING_2026-08-12.md
19_CONTROLLED_RECOVERY_AND_ARCHIVE_SCALING_2026-08-12.md
20_AUTONOMOUS_ARCHIVE_PAGING_2026-08-12.md
21_AUTONOMOUS_ARCHIVE_BOOT_AND_APPEND_SCALING_2026-08-12.md
22_WORKSHOP_REGISTRY_SCALING_2026-08-12.md
```

Ключевые safety результаты:

```text
TIMED_OUT → MANUAL_REVIEW_REQUIRED
manual-review / ambiguous persisted state → backup blocked
linked recovery → exact snapshot + exact immutable spool-selection
wrong spool/session/run → writeoff rejected before warehouse mutation
RUN_COMPLETED alone → no automatic wire writeoff
```

## Autonomous Arduino archive — scaling state

Authoritative append-only files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

### Deep/boot audit

```text
events       O(E)
assignments  fixed batches of 32 references
```

`AutonomousWindingArchive::begin()` использует тот же authoritative `validateStorage()`.

### HTTP/UI

```text
GET /api/autonomous-windings
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
```

Cursor не разделяет normal `RUN_STARTED/RUN_COMPLETED` pair.

### Runtime append

`LOCAL_EVT` replay/completion использует bounded tail-state, а не full history scan.

Непустой NDJSON обязан заканчиваться `\n`; interrupted append fail-closed.

## Workshop registry — scaling state

Authoritative files:

```text
/data/workshop/clients.ndjson
/data/workshop/motors.ndjson
/data/workshop/repairs.ndjson
/data/workshop/repair-status.ndjson
/data/repairs/pricing.ndjson
```

### Bounded list readers

Добавлены cursor pages max 32 для:

```text
/api/clients
/api/motors
/api/repairs
```

Если request содержит `limit` или `cursor`, response содержит:

```text
count
limit
cursor
has_more
next_cursor
max_page_size
```

Основные mobile/desktop clients, motors, repairs и Arduino archive уже используют paged reads.

Legacy GET без `limit/cursor` временно оставлен, потому что ещё есть старые страницы, которые ожидают полный list response. Не удалять его до окончания consumer migration.

### Exact lookup

Добавлены:

```text
GET /api/clients/by-id?client_id=...
GET /api/motors/by-id?motor_id=...
GET /api/repairs/by-id?repair_id=...
```

Mobile costing уже использует exact lookup и больше не перечисляет всю business database ради одной карточки.

Desktop costing и как минимум один winding-job screen на текущем checkpoint ещё используют legacy full GET; их мигрировать до bounded-default switch.

### Business-data integrity / boot

Старый O(n^2) audit заменён authoritative bounded-complexity audit:

```text
client IDs      strict monotonic append-only pass
motor IDs       strict monotonic pass + coil_program validation
repair IDs      strict monotonic pass
repair refs     batches of 32
CLOSED status   batches of 32 + exact-one status occurrence
pricing refs    batches of 32
```

`RepairRegistry::begin()` теперь использует:

```text
BackupBusinessDataIntegrityAudit::checkWorkshopRegistry()
```

и больше не запускает legacy nested validators.

`nextId()` также один раз проходит monotonic ID ledger вместо O(n^2) duplicate scans.

## UI deployment note

Изменены несколько файлов `firmware/esp32/web/mobile` и `desktop`.

После успешного firmware build/upload полностью заменить microSD `/web` актуальным repo `firmware/esp32/web`. Не делать overlay поверх старого `/web`.

## Следующее действие

Перед дальнейшим крупным firmware refactor нужен clean build текущего HEAD:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

После `SUCCESS` продолжить:

1. мигрировать desktop costing и оставшиеся legacy registry consumers на exact/paged reads;
2. сделать clients/motors/repairs bounded mode default и удалить unbounded formatter path;
3. оптимизировать repair status lookup внутри repair pages только если build/runtime текущего batch успешен;
4. снять populated-dataset timings через `/api/backup/manifest`;
5. segmentation/rotation thresholds вводить только по фактическим latency и размерам файлов;
6. DB migration/destructive compaction не вводить без доказанной необходимости.

Основная новая функциональность сейчас не приоритет. Проект находится в production-hardening/performance phase.
