# 83 — Material backup scoped audit

Дата: 2026-08-25
Ветка: `cmp-protocol-v1`

## Что изменено

Stage-1 NDJSON performance review подтвердил старый transitive duplicate hotspot в `MaterialPersistenceIntegrityAudit`.

Раньше metrics-overload, который вызывает deep backup manifest, после проверки:

- `materials.ndjson`;
- `usage.ndjson`;
- `adjustments.ndjson`;

дополнительно транзитивно запускал:

- `WorkshopPersistenceIntegrityAudit::check(storage)`;
- `RepairPricingIntegrityAudit::check(storage)`.

Сразу после этого `CM_BackupExportWeb.cpp` отдельно выполняет authoritative `BackupBusinessDataIntegrityAudit::check(storage, businessMetrics)`, который снова валидирует workshop/business + repair pricing domain. Это давало доказанный лишний I/O в каждом deep backup snapshot stability pass.

## Новая структура

Добавлен:

```cpp
MaterialPersistenceIntegrityAudit::checkMaterialDomain(storage, metrics)
```

Он валидирует material domain и exact references, но сам не запускает broad workshop/pricing audits.

Семантика публичных overloads теперь разделена намеренно:

- `check(storage)` — остаётся broad standalone fail-closed audit и после material-domain validation по-прежнему запускает `WorkshopPersistenceIntegrityAudit` + `RepairPricingIntegrityAudit`;
- `check(storage, metrics)` — scoped composite-backup overload; он выполняет только material domain, потому что backup manifest следующим authoritative шагом выполняет `BackupBusinessDataIntegrityAudit`.

Code-search audit подтвердил, что metrics-overload production-кода используется именно в `CM_BackupExportWeb.cpp`. Поэтому изменение не ослабляет другие standalone callers.

## Safety / integrity

Не менялись:

- material parser/schema validation;
- exact material and repair reference checks;
- arithmetic validation usage/adjustments;
- fail-closed backup behavior;
- authoritative business/pricing validation внутри того же snapshot stability flow;
- physical START/SSR;
- RUN_COMPLETED/manual write-off semantics;
- exact spool/session/run provenance.

## Commits

- `c56bc7e949c8f5197e71121d48fc3d7e5d996183` — expose scoped material-domain API.
- `20a1a88cbc7389aafcc7a5ee663f6f5d925f99f3` — split scoped implementation while preserving broad standalone behavior.
- `79d439f3cb236281e7c62863bc294a43c189bf1e` — make metrics overload scoped for composite backup audit.
- `bd3a4e91dfe3bbfdd6302a16f4007cb416c41851` — regression contract.
- `22119a12df8194c80dd9bba1715f2c16873f83ac` — CI step `Audit material backup scoped contracts`.

## Проверка

Software CI ещё требуется:

- ESP32 Build on `79d439f3...` or descendant;
- CMP Protocol Tests on `22119a12...` or descendant;
- особенно `Audit material backup scoped contracts`, material ledger/final acceptance/kg-first/backup-related audits.

Hardware test для этого storage-only refactor не требуется.
