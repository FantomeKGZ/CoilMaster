# Где остановились и что делать дальше

Дата обновления: **2026-08-12**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Подтверждённый production hardware checkpoint

На реальном стенде подтверждены оба пути.

Standalone Arduino:

```text
локальное создание программы на Arduino
→ physical START
→ реальная намотка
→ LOCAL_EVT
→ ESP32 autonomous archive
```

Основной linked production flow:

```text
client / motor / OPEN repair
→ linked winding
→ exact spool selection
→ UART delivery
→ physical START
→ реальная намотка
→ RUN_STARTED / RUN_COMPLETED
→ manual exact-run wire writeoff
→ данные списания отображаются
→ costing / finalization
→ CLOSED / итоговые данные находятся
→ backup/manifest читается
```

Основной happy-path hardware E2E **закрыт** и больше не считается внешним неподтверждённым риском.

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

## Build / runtime checkpoints

Предыдущий clean ESP32 build был подтверждён пользователем:

```text
RAM:   14.4% (used 47320 bytes from 327680 bytes)
Flash: 86.7% (used 1136229 bytes from 1310720 bytes)
SUCCESS Took 31.04 seconds
```

После него manifest на реальном ESP32 подтвердил:

```text
export_allowed=true
activity_state_verified=true
snapshot_stability_checked=true
snapshot_stable=true
snapshot_stability_reason=null
snapshot_stability_duration_ms=1429
```

Baseline сохранён в:

```text
docs/PROJECT_HANDOFF/14_HARDWARE_MANIFEST_BASELINE_2026-08-12.md
```

Полный hardware production E2E сохранён в:

```text
docs/PROJECT_HANDOFF/15_HARDWARE_PRODUCTION_E2E_2026-08-12.md
```

## Autonomous Arduino archive

Persistent files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

ESP32 parser fail-closed требует:

```text
RUN_STARTED   -> completed_runs == 0
RUN_COMPLETED -> completed_runs > 0
```

Backup whitelist/deep audit включает autonomous events + assignments и публикует:

```text
autonomous_winding_archive_audit_duration_ms
autonomous_winding_event_record_count
autonomous_winding_started_record_count
autonomous_winding_completed_record_count
autonomous_winding_assignment_record_count
```

Исторический `STARTED_NOT_COMPLETED` не трактовать автоматически как физически активную намотку после reboot: архив может содержать старые прерванные задачи. Для authoritative post-reboot Arduino state потребуется отдельный handshake/recovery protocol, если такой блок будет реализовываться.

## Fault hardening — текущий блок

После hardware E2E начат negative/fault pass.

Закрыт конкретный reboot/backup gap:

```text
f9e405b08a30d1d9f8413655c10cda6acf090b79  Block backup during manual recovery review
817d1eb0040a1164ed808fc325252fb65d648aa8  Fail closed on faulted backup activity
```

До исправления `backupRuntimeActivity()` возвращал `Safe` для `MANUAL_REVIEW_REQUIRED`. Это было слишком оптимистично: после reboot ESP32 не может доказать, что Arduino физически idle.

Теперь:

```text
MANUAL_REVIEW_REQUIRED -> BackupActivityCheck::Busy
```

а fallback по persisted state также считает небезопасным:

```text
Created
Delivering
WaitingPhysicalStart
Running
Fault
```

Heavy backup/deep audit остаётся заблокирован до явного operator recovery/closure. Никакого auto-resume, physical START или writeoff это изменение не добавляет.

## Verification status текущего HEAD

Предыдущий ESP32 build и production E2E подтверждены, но после двух новых fault-hardening commits текущий HEAD ещё не пересобран:

```text
CURRENT HEAD BUILD: NOT CONFIRMED
CI: NOT CONFIRMED
```

GitHub CI не считать green без фактического result.

## Следующее действие

Сначала clean-build текущего ESP32 HEAD:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

После успешного build прошить:

```powershell
pio run -e esp32 -t upload
```

Ближайший безопасный negative test при следующей короткой намотке:

```text
RUN_STARTED / run_active=true
→ запрос GET /api/backup/manifest
→ export_allowed MUST be false
→ snapshot_stability_checked MUST be false
→ heavy deep audit не должен запускаться
```

Reboot/manual-review test проводить отдельно и только контролируемо, не прерывая рабочий двигатель без необходимости. После reboot из persisted `Running/Delivering/WaitingPhysicalStart/Fault` backup должен оставаться blocked до operator review.

## Оставшиеся production-hardening сценарии

- reboot/manual-review controlled test;
- microSD loss / unavailable storage;
- corrupted persisted data;
- UART timeout / reject / duplicate event;
- wrong spool / session / run;
- duplicate writeoff;
- close без required writeoff coverage;
- backup request во время active winding;
- populated-dataset benchmark перед Stage 1 performance work.

Основная новая функциональность сейчас не приоритет. Следующий этап — доказать fail-closed поведение этих сценариев и исправлять только найденные реальные gaps.
