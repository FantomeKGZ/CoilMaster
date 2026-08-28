# Checkpoint 152 — RUN_WIRE Material Request status batching

Дата: **2026-08-28**  
Рабочая ветка: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — не изменён.

## Проблема

RUN_WIRE web-preflight раньше загружал bounded страницы `/api/material-requests`, а затем для каждой строки делал отдельный GET `/api/material-requests/status`. Каждый status lookup повторно проходил immutable request journal и status journal. При росте истории это давало N+1 server-side scans.

## Реализация

`MaterialRequestStatusStore` получил bounded batch resolver:

```text
MaxBatchSize = 24
resolveBatch(ids, count, states, found)
```

Для одной bounded страницы он выполняет:

1. один полный streamed scan `/data/workshop/material-requests.ndjson`;
2. один полный streamed scan `/data/workshop/material-request-status.ndjson`.

Сохранены fail-closed проверки:

- положительные и уникальные request ids;
- global monotonic `material_request_id`;
- global monotonic `transition_id`;
- только допустимые DRAFT -> ISSUED -> PRICED -> CLOSED переходы;
- exact per-request `from_status` chain для каждого запрошенного id;
- transition-count overflow guard;
- отсутствие `std::vector`, whole-file `readString()` и tail-only shortcut.

Добавлен read-only endpoint:

```text
GET /api/material-requests/status-batch?ids=1,2,...
```

Максимум 24 id. Mutation semantics `/api/material-requests/status` POST не изменены.

## RUN_WIRE web bridge

Чтобы не переписывать уже проверенный `writeoff-spool-suggestion.js`, добавлен:

```text
/shared/material-request-status-prefetch.js
```

Он загружается перед основным RUN_WIRE controller в desktop/mobile `writeoff.html`.

На каждой bounded странице Material Request он:

- проверяет positive/unique request ids;
- делает один status-batch GET;
- проверяет exact id/order/count/status domain;
- временно обслуживает старые per-item status GET из page cache;
- потребляет каждую cache запись ровно один раз (`statusCache.delete(id)`), чтобы состояние не переживало текущий preflight;
- учитывает как `init.method`, так и `Request.method`;
- при batch/read/integrity mismatch fail-closed и не откатывается к N+1 path.

Prefetch layer не содержит POST и не вызывает warehouse mutation endpoint.

## RUN_WIRE safety semantics не изменены

Основной controller по-прежнему вручную отправляет только:

```text
confirmed = true
source_kind = RUN_WIRE
exact source_session_id
exact source_run_id
exact immutable spool_id
wire diameter/material identity
```

Mutation endpoint остаётся:

```text
POST /api/material-requests/warehouse
```

RUN_WIRE не маршрутизируется через generic `/api/materials/usage`.
`RUN_COMPLETED` остаётся только evidence.

## Коммиты

```text
32e284a1a0cc4073c4565a7fccb018839384e87f  bounded batch API declaration
7ee1b19586bf62dcf82eee9ec92335bf1c193c1d  one request scan + one status scan batch resolver
515cfd38174878d59907c7b79774273e11ba6d39  Web handler declaration
6989e838358e2277c0d1f6c0c42f6baec8721d6d  bounded status-batch endpoint
ed41cba9d1292d9dac6662f91fdf8c69b0d3c513  shared status prefetch foundation
4b491d6c3590a050a5afb02a5457fa4e25d274a9  desktop integration
635c0c4f4ede62630c47590d9380670ee91cf83c  mobile parity
5785799812eb1fd987242b592638021cfd06a50c  status-store contract extension
6710acb21ccc84cb2e62f05566bb759e0e4dff7f  mandatory CMP step
104f319e1cb8e466a229c0d40876b35aac6ded86  one-shot cache + Request.method safety
17c7c43058802f44d5f0b26d0da301190e792ebd  final mandatory contract
```

Intermediate CMP #3882 failed only because the new contract asserted one exact JS guard spelling; runtime/other audits were green. The assertion was made semantic/formatting-agnostic without runtime rollback.

## Verification — GREEN

```text
CMP Protocol Tests #3885
run 33167039155
SHA 17c7c43058802f44d5f0b26d0da301190e792ebd
completed / success

ESP32 Build #1708
run 33167009269
SHA 104f319e1cb8e466a229c0d40876b35aac6ded86
completed / success

Arduino RU LCD Build #132
run 33167009264
SHA 104f319e1cb8e466a229c0d40876b35aac6ded86
completed / success
```

## Следующий repo-safe аудит

Продолжать repeated-scan review без изменения safety contracts. Первый кандидат: RUN_WIRE exact spool lookup — проверить, не проходит ли Web весь paged spool catalogue ради одного immutable `spool_id`, и существует ли уже authoritative bounded/by-id backend lookup, который можно безопасно переиспользовать.

Hardware E2E остаётся отдельным финальным gate.
