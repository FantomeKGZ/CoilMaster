# CoilMaster — Material Request lifecycle

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

## Что реализовано

Добавлен append-only журнал статусов:

```text
/data/workshop/material-request-status.ndjson
```

Identity заявки в `material-requests.ndjson` остаётся immutable и содержит `initial_status=DRAFT`.

Разрешённая цепочка:

```text
DRAFT -> ISSUED -> PRICED -> CLOSED
```

`MaterialRequestStatusStore::resolve()` восстанавливает текущий статус по истории. Успешный lookup отсутствующей заявки возвращает `true` + `found=false`; storage/validation failure возвращает `false`, чтобы Web/API мог отличать 404 от 5xx.

## Backup / integrity

- `material-request-status.ndjson` добавлен в backup export;
- CRM integrity audit проверяет структуру status events, monotonic `transition_id`, legal transitions и ссылки на существующие material requests;
- audit movement units синхронизирован с production store и принимает `M2`;
- audit остаётся read-only/fail-closed.

## Regression

Постоянные проверки:

```text
Tests/Web/check_material_request_status_store.js
Tests/Web/check_material_request_status_backup.js
```

Обе включены в `.github/workflows/cmp-protocol-tests.yml`.

## Основные commits

```text
f0c2bd491072c8525a72adaf446c8380a1ea06b1  lifecycle store implementation
200d6a7a3ee0a0406031e7540b7c6d06ea02be2d  missing-request lookup semantics
973f330c9ff317aeddeb6e29e63b6743e1c06af2  lifecycle regression
466c4374f975c04b27129d49d5653a1552f5183f  backup/integrity + M2 audit sync
```

One-shot workflow used for guarded large-file patch is archived as manual-only/read-only; it cannot push changes.

## Verification

Final verification head:

```text
a960999b040afbdd7c48bbde08763e042408a2e8
```

Evidence:

```text
CMP Protocol Tests run 32860049965 / SUCCESS
ESP32 Build run 32860049946 / SUCCESS
```

CMP run includes both new lifecycle tests.

## Safety

Не изменено:

- `RUN_COMPLETED` never auto-deducts material;
- physical warehouse ISSUE remains explicit operator action;
- current exact-spool production flow remains authoritative until coordinated migration;
- no silent status rewrite/delete;
- corrections/returns remain append-only target semantics.

## Следующий блок

Crash-safe Material Request warehouse transaction coordinator for explicit:

```text
ISSUE
RETURN
CORRECTION
```

Он должен связать physical `MaterialLedger` stock mutation и immutable Material Request movement evidence без crash-window между двумя журналами.
