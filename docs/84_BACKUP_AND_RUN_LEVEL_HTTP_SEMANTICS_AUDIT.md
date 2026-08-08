# Backup и run-level HTTP/error semantics audit

Дата: 2026-08-08  
Ветка: `cmp-protocol-v1`

## Scope

Проверены новые read-only backup endpoints и run-level API, которые опираются на persisted winding provenance:

```text
GET  /api/backup/manifest
GET  /api/backup/file?name=...
GET  /api/backup/sessions
GET  /api/backup/session-file?kind=...&session_id=...
GET  /api/winding-history?...filters...
POST /api/warehouse/write-offs
```

Repository review не заменяет hardware E2E ESP32 + Arduino.

## Каноническая семантика статусов

```text
400  malformed / missing required request field
404  requested allowed resource does not exist
409  request syntactically valid, but current domain/machine state forbids it
500  persisted data/read/integrity failure while dependency is otherwise available
503  storage/service/dependency unavailable
```

## Backup endpoints

### `/api/backup/manifest`

Manifest намеренно отличается от direct export endpoints:

- storage unavailable -> `503`;
- active winding не превращает manifest в `409`: manifest остаётся read-only status endpoint и возвращает `200` с `export_allowed=false`;
- если activity state не доказан, manifest возвращает blocked state и не запускает deep scan;
- `snapshot_stability_checked=false` означает, что тяжёлый integrity audit намеренно не выполнялся;
- `snapshot_stable=null` допустим только когда deep scan не выполнялся;
- `snapshot_stability_duration_ms=null` допустим только когда deep scan не выполнялся; при `snapshot_stability_checked=true` поле содержит фактическую длительность уже выполненного deep audit в миллисекундах и не запускает дополнительный filesystem scan;
- `winding_journal_record_count` возвращается только если winding schema validation **и** transition audit полностью успешны; если deep scan не запускался, до winding audit не дошли или winding integrity не доказана, значение `null`;
- при safe state integrity failure возвращается как `snapshot_stable=false` + `snapshot_stability_reason`, а не как транспортная ошибка HTTP.

Это разделяет operational state и failure чтения самого endpoint. Duration/count поля являются observability metadata и не участвуют в решении `snapshot_stable`/`export_allowed`.

`winding_journal_record_count` считается внутри уже существующего `WindingJournalQuery::validateAll()` прохода до EOF. Дополнительного чтения `events.ndjson` ради метрики нет.

### `/api/backup/file`

- отсутствующий `name` -> `400`;
- имя вне whitelist -> `404 backup_file_not_allowed`;
- разрешённый, но отсутствующий файл -> `404 backup_file_not_found`;
- active winding -> `409 backup_blocked_while_winding_active`;
- activity state unavailable / storage unavailable -> `503`;
- open/read failure -> `500`.

### `/api/backup/sessions`

- невалидный `after_session_id` / `limit` -> `400`;
- session temp marker -> `409` как нестабильный snapshot state;
- session directory unavailable -> `503`;
- malformed/non-canonical session directory -> `500`;
- session file metadata read failure -> `500`.

Пустые optional query values (`after_session_id=` / `limit=`) сейчас трактуются как omitted/default. Это оставлено как compatibility behavior; оно не даёт доступа к произвольным paths и не ослабляет integrity checks.

### `/api/backup/session-file`

- отсутствуют `kind`/`session_id` -> `400`;
- invalid kind/session ID -> `400`;
- canonical allowed session file отсутствует -> `404`;
- storage/read failure -> `500/503` согласно доступности dependency;
- active winding -> `409` через общий backup activity guard.

## Winding history

`GET /api/winding-history` сохраняет единый контракт:

- ровно один из `session_id` / `repair_id` обязателен;
- malformed filter/cursor/limit -> `400`;
- storage/query unavailable -> `503`;
- journal read/schema failure -> `500`;
- success -> `200`.

Cursor pagination остаётся только пользовательским history API. Deep winding backup audit её не использует: full-file schema validation выполняется authoritative `WindingJournalQuery::validateAll()`.

## Run-level write-off provenance

В `POST /api/warehouse/write-offs` найден реальный semantic gap: если клиент присылал одновременно

```text
source_session_id=
source_run_id=
```

старый код считал оба поля отсутствующими и мог перейти в legacy write-off path без run provenance.

Исправлено: само присутствие одного/both run-level параметров теперь включает строгую provenance validation. Оба параметра должны присутствовать вместе, быть непустыми и содержать canonical non-zero IDs; иначе -> `400` без записи.

Коммиты, связанные с этим audit/observability batch:

```text
362fcb7daa8f883f57de4867c06c42f06e45b613  Reject empty run provenance fields
8b61f46e1cb9d866bf9aa94800dd6a95f347c6b0  Measure deep backup audit duration
c35b87717f7b64178f7c942f0228bd301771a78e  Expose winding journal validation count
36e0aee29506be33608f42bb2d7bfca87713b280  Count records during winding journal validation
eeea77a35e0938692f2142b5022ea41849bf5f64  Expose winding audit record count
1101ab18ef6a39e087e5f3b62814ec5d584b871c  Return validated winding record count
a1aa70381f53d10578fbb483a1335a96c8818551  Expose winding journal count in backup manifest
b84da0162ba73492742a261807c645eb1263b44b  Make winding audit count type explicit
```

Остальные run-level conflicts остаются `409`: repair closed, spool/session mismatch, run not completed, duplicate confirmed write-off. Persisted history/integrity failure остаётся `500`, dependency unavailable -> `503`.

## Результат

HTTP/error semantics после исправления согласованы с fail-closed моделью. Дополнительный массовый refactor status codes сейчас не нужен.

Performance Stage 0 теперь имеет две runtime observability величины без дополнительного persistence scan: полную длительность deep backup audit и число успешно провалидированных записей winding journal. Rotation/storage refactor до измерений не начинать.
