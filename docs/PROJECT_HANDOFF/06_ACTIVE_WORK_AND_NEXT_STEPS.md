# Где остановились и что делать дальше

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Hardware production status

На реальном стенде подтверждены оба основных пути.

Standalone Arduino:

```text
local program on Arduino
→ physical START
→ real winding
→ LOCAL_EVT
→ ESP32 autonomous archive
```

Linked production:

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

Arduino pin-map:

```text
D11 = Buzzer
D12 = SSR
A0  = Hall
A1  = Arduino TX → ESP32 RX
A2  = Arduino RX ← ESP32 TX
```

## Verification status текущего HEAD

Последний подтверждённый пользователем ESP32 build был до текущих paging/archive-scaling изменений:

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

## Backup / integrity hardware baseline

Ранее на реальном ESP32 подтверждено:

```text
export_allowed=true
activity_state_verified=true
snapshot_stability_checked=true
snapshot_stable=true
snapshot_stability_reason=null
snapshot_stability_duration_ms=1429
```

Документы:

```text
docs/PROJECT_HANDOFF/14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
docs/PROJECT_HANDOFF/15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
docs/PROJECT_HANDOFF/16_HARDWARE_NEGATIVE_BACKUP_ACTIVE_2026-08-12.md
```

## Закрытый fault-hardening

### Backup / reboot

`MANUAL_REVIEW_REQUIRED` и ambiguous persisted states fail-closed блокируют backup.

### Runtime storage corruption

Allocator/state paths заново проверяются перед новой session; `.tmp`, повреждённые state/high-water и неоднозначный recovery блокируют job creation.

### UART timeout / replay

```text
TIMED_OUT
→ MANUAL_REVIEW_REQUIRED
→ new job blocked
→ backup blocked
→ no auto resend / auto resume
```

Linked duplicate требует exact event semantics. Autonomous duplicate требует exact session/run/event/completed/program semantics.

### Controlled recovery

Оба recovery endpoint перед `CLOSED_AFTER_REVIEW` перепроверяют persisted latest state + immutable snapshot; linked job дополнительно требует exact immutable spool-selection. Recovery closure выполняет ESP32 restart.

Backup activity guard повторно доказывает persisted state/snapshot/spool identity даже при runtime `Safe`.

Документы:

```text
docs/PROJECT_HANDOFF/17_RUNTIME_STORAGE_CORRUPTION_HARDENING_2026-08-12.md
docs/PROJECT_HANDOFF/18_UART_TIMEOUT_REPLAY_HARDENING_2026-08-12.md
docs/PROJECT_HANDOFF/19_CONTROLLED_RECOVERY_AND_ARCHIVE_SCALING_2026-08-12.md
```

## Autonomous Arduino archive — current scaling state

Authoritative files остаются append-only:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

DB migration, destructive compaction и automatic rotation не вводились.

### Deep audit

```text
events       O(E)
assignments  fixed batches of 32 references
```

### HTTP/UI paging

`GET /api/autonomous-windings`:

```text
limit default 20
limit max 32
cursor opaque byte offset
has_more
next_cursor
```

Cursor выдаётся только на logical task boundary и не разрезает normal `RUN_STARTED/RUN_COMPLETED` pair.

Mobile/Desktop:

```text
20 tasks
→ Показать ещё
→ next_cursor
```

Полный проход для «Скомплектованные двигатели» запускается только явно и также идёт bounded pages.

### Boot validation

`AutonomousWindingArchive::begin()` больше не вызывает legacy repeated validators.

Теперь:

```text
ensureDirectories()
→ authoritative validateStorage()
```

Legacy unbounded formatter/validators удалены из production class/code.

### Runtime LOCAL_EVT append

`save()` больше не перечитывает весь `events.ndjson` для каждого replay/completion.

Используется bounded tail-state:

```text
old run_id                        → INVALID
same run exact replay             → DUPLICATE
same run START → COMPLETE         → allowed
same run conflicting semantics    → INVALID
new monotonic run                 → append
```

Таким образом normal UART append/retry latency больше не растёт линейно вместе со всей history.

### Power-loss tail integrity

Непустые `events.ndjson` и `assignments.ndjson` обязаны заканчиваться `\n`; unterminated append fail-closed отклоняется boot/deep audit.

Документы:

```text
docs/PROJECT_HANDOFF/20_AUTONOMOUS_ARCHIVE_PAGING_2026-08-12.md
docs/PROJECT_HANDOFF/21_AUTONOMOUS_ARCHIVE_BOOT_AND_APPEND_SCALING_2026-08-12.md
```

## UI deployment note

Paging изменил:

```text
firmware/esp32/web/mobile/arduino-windings.html
firmware/esp32/web/desktop/arduino-windings.html
```

После успешного firmware build/upload заменить microSD `/web` актуальным содержимым repo `firmware/esp32/web` целиком, а не overlay поверх старой версии.

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

и заменить `/web` на microSD.

Минимальная runtime проверка после upload:

```text
1. local Arduino task → physical START → RUN_COMPLETED → LOCAL_EVT saved
2. autonomous archive page opens
3. GET /api/autonomous-windings?limit=1
4. if has_more=true, request next_cursor and verify no duplicate/split pair
5. GET /api/backup/manifest → snapshot_stable=true on intact SD
```

## Следующий repo-reviewable шаг после build

1. снять populated-dataset metrics:
   - autonomous event/assignment record counts;
   - autonomous audit duration;
   - total backup stability duration;
   - real NDJSON file sizes;
2. сравнить с baseline 1429 ms;
3. только после metrics решать, нужен ли segmentation/rotation threshold;
4. не вводить DB migration/destructive compaction без доказанной необходимости.

Основная новая функциональность сейчас не приоритет. Проект находится в production-hardening/performance phase.
