# Checkpoint 163 — completed Web-job assignment proof reuse

Дата: **2026-08-29**  
Ветка: **`arduino-ru-lcd-experiment`**  
Production `cmp-protocol-v1` не изменён.

## Статус

**GREEN**.

## Что оптимизировано

В `AutonomousWindingArchive::assignCompletedWebJobMotorChecked(..., replaceExisting, ...)` уже выполнялась authoritative validation завершённого ESP32/Web job через persisted `JobStateStore` + immutable `JobSnapshotStore`. После canonical motor-winding projection старый assignment overload повторно читал те же state/snapshot перед append.

Добавлен узкий private path `assignCompletedWebJobMotorKnownTaskChecked(...)`, который используется только после успешной проверки state/snapshot в том же assignment flow. Он не повторяет уже доказанные immutable reads.

При этом mutation-time `web-assignments.ndjson` scan сохранён полностью непосредственно перед append:

- flat JSON/schema validation;
- strict monotonic `assignment_id` ordering;
- exact `session_id + run_id` retry lookup;
- duplicate exact-key corruption detection;
- motor/role conflict rejection;
- idempotent exact retry;
- highest-id allocation from того же authoritative pass;
- append + flush + exact write-length check.

Canonical projection, role replacement semantics и retry provenance не изменены.

## NO-CHANGE рядом

Local-autonomous assignment path сохраняет повторный `completedTaskExists(sessionId, runId, ...)` перед append. Этот reread является authoritative task proof на assignment mutation/TOCTOU boundary и не удаляется ради производительности.

## Коммиты

```text
67b26bfc62e7c28be2883b200e411ce7dec4a9ad  add known completed-job assignment append path
70d8229fb649e21aa303f5627dd5ac14df0e0e27  reuse validated completed job on assignment
c60f655a8f11ac3a82a1e29a59ab46989d7eb92b  avoid state/snapshot reread after projection
27bf45c6cdd13757b49b2127bb11af616b2e1c10  regression contract
```

## Проверенный CI

Runtime `c60f655a8f11ac3a82a1e29a59ab46989d7eb92b`:

```text
CMP Protocol Tests #3998  run 33264241295 / SUCCESS
ESP32 Build #1769         run 33264241291 / SUCCESS
Arduino RU LCD #193       run 33264241294 / SUCCESS
```

Regression contract `27bf45c6cdd13757b49b2127bb11af616b2e1c10`:

```text
CMP Protocol Tests #3999  run 33264263872 / SUCCESS
```

## Safety / integrity

Не изменены:

- physical START / resume semantics;
- Arduino SSR ownership;
- RUN evidence;
- manual RUN_WIRE writeoff и exact spool/session/run provenance;
- canonical append-only winding history;
- assignment-ledger mutation-time validation;
- recovery/TOCTOU boundaries;
- bounded RAM policy;
- production history/rotation policy.

Никаких persistent cache/index/DB, whole-file buffering, automatic truncation или production transfer не добавлено.
