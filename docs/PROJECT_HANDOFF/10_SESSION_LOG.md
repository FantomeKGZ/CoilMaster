# Журнал рабочих сессий

Этот файл обновляется после каждой рабочей сессии.

## Формат записи

```text
## YYYY-MM-DD HH:MM — Название

Цель:

Что сделано:

Файлы:

Коммиты:

Проверка:

Где остановились:

Следующее действие:
```

---

## 2026-08-08 18:50 — Winding cleanup verification и material backup observability

Цель:

Продолжить с обязательного cleanup `CM_WindingPersistenceIntegrityAudit`, затем перейти к следующему repo-reviewable улучшению без изменения safety-инвариантов и синхронизировать handoff с фактическим HEAD `cmp-protocol-v1`.

Что сделано:

- перепроверен актуальный `CM_WindingPersistenceIntegrityAudit.cpp`: старого cursor-pagination full scan уже нет;
- подтверждено использование `WindingJournalQuery::validateAll(recordCount)` и отдельного `WindingJournalTransitionAudit::validate()`;
- найден отдельный implementation unit `CM_WindingJournalQueryValidation.cpp`, где реализованы оба `validateAll()` overloads; key-files index обновлён, чтобы следующий review не искал реализацию только в `CM_WindingJournalQuery.cpp`;
- не дублировался `CM_WindingSessionPersistenceIntegrityAudit`, который остаётся authoritative deep parser/cross-file identity audit для snapshot/state/spool-selection;
- в текущем HEAD уже присутствовал следующий same-pass observability batch для material persistence;
- `MaterialPersistenceIntegrityAudit` теперь считает catalogue/usage/adjustment records в существующих validation loops и сохраняет старый `check(storage)` contract;
- backup manifest публикует `material_catalog_record_count`, `material_usage_record_count`, `material_adjustment_record_count` вместе с winding/warehouse counts и общей duration;
- static integration review подтвердил explicit public includes, compatibility overload, отсутствие публикации partial material counts при failed full audit и неизменённый `BackupActivityGuard::Safe` gating;
- root `platformio.ini` подтверждает `+<firmware/esp32/src/*.cpp>`, поэтому отдельный `CM_WindingJournalQueryValidation.cpp` входит в ESP32 build source filter;
- выявленный transitive duplicate-I/O path `MaterialPersistenceIntegrityAudit → WorkshopPersistenceIntegrityAudit` зафиксирован как measured Stage 1 candidate, но не рефакторился до hardware benchmark;
- синхронизированы `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `09_KEY_FILES_INDEX.md`.

Кодовые commits material observability, уже находившиеся в актуальной ветке в ходе этой сессии:

```text
ac031d8cc14a74786e10c8adb782776b0d16e97f  Expose material persistence audit counts
6cf4ad7da157c8e65f131b9a851c4243c0914e31  Count material persistence audit records
38befe338cfc57879d2ad09fc6be54d54c190441  Expose material audit counts in backup manifest
0f10ed32d110c28b21af7c46c6be20a084c6ba2b  Document material backup observability
899508534fc909d9baacada62bcda8629ddf0b4a  Advance active work to material observability
```

Документационные commits этой сессии:

```text
1a78e23bdfc3addb7826845fc8273b33d96f5e67  Record material backup observability in current state
9f32a1c953017447a71d907535ac2cf0398a5098  Refresh backup and winding key file index
5a7070d805eb902e11046b7dfe94af169d549e99  Record material metrics integration review
```

Проверка:

Repository-level static review подтверждает текущую wiring/compatibility, но не заменяет фактический ESP32 build. GREEN CI для текущего head в этой записи не подтверждён. Hardware E2E также не выполнялся и остаётся внешним обязательным этапом.

Где остановились:

Deep backup integrity и winding `validateAll()` cleanup завершены. Stage 0 observability теперь включает material/winding/warehouse record counts и общую deep-audit duration без второго full-file scan ради метрик. До измерений не начинать rotation/database/persistent-cache и не делать Stage 1 duplicate-scan refactor только ради эстетики.

Следующее действие:

1. Реальный ESP32 + Arduino E2E production path.
2. На backup этапе снять `materials.size_bytes`, `material_catalog_record_count`, `material-usage.size_bytes`, `material_usage_record_count`, `material-adjustments.size_bytes`, `material_adjustment_record_count`, `winding-events.size_bytes`, `winding_journal_record_count`, `warehouse-movements.size_bytes`, `warehouse_movement_record_count`, `snapshot_stability_duration_ms`.
3. По фактическим latency/size выбрать hotspot и только после этого решать bounded in-request index / duplicate-audit decomposition / rotation.
4. Если появляется новый Actions failure — читать фактический `build-esp32` log и исправлять первую реальную compile/link error.

---

## 2026-08-08 12:35 — Deep backup integrity, Actions diagnosis и handoff

Цель:

Довести read-only backup/export от простого whitelist-download до доказуемого snapshot integrity, найти реальную причину красного ESP32 Actions run и сохранить полную точку продолжения для нового чата.

Что сделано:

- добавлен строгий read-only audit workshop clients/motors/repairs, repair-status и repair-pricing references;
- добавлен read-only winding persistence audit;
- глубокий manifest audit ограничен состоянием `BackupActivityGuard::Safe`, чтобы во время active winding не делать длинные scans microSD;
- подключены warehouse persistence, warehouse movement, material persistence, allocator, conductor settings и session persistence audits;
- `snapshot_stability_checked=false` / `snapshot_stable=null` при небезопасном activity state;
- backup UI mobile/desktop получил operator-readable причины instability;
- зелёный `snapshot_stable` теперь требует глубокую проверку практически всего экспортируемого persistent-набора и cross-file identity;
- получен полный job log Actions run `31243187630`, job `93067378338`;
- подтверждено, что `WString.h [-Wconversion]` — warnings, не причина failure;
- реальная ошибка: `CM_MaterialLedger.cpp:705: expected '}' at end of input`;
- в текущей ветке добавлена отсутствующая closing brace namespace `CM`;
- создан полный свежий handoff `12_LATEST_HANDOFF_2026-08-08.md`;
- `00_READ_FIRST.md` перенаправлен на новый handoff.

Ключевые коммиты этой части:

```text
1a21073f  Add backup business data integrity audit contract
b9236557  Implement backup business data integrity audit
5a15dbfa  Audit workshop and pricing in backup manifest
0b68f3ce  Explain business data backup instability on mobile
f6cdd6d7  Explain business data backup instability on desktop
11191769  Add winding persistence integrity audit contract
1406c73f  Implement winding persistence integrity audit
fe988944  Audit winding persistence only when backup is safe
bbd0a507  Explain winding backup instability on mobile
21d5ab1b  Explain winding backup instability on desktop
b8ee44ce  Audit warehouse persistence in backup manifest
80958209  Explain warehouse persistence backup instability on mobile
e0d19ebe  Explain warehouse persistence backup instability on desktop
70220e54  Audit persistent allocator state in backup manifest
92a6a11e  Add conductor settings integrity audit contract
8f5cc608  Implement conductor settings integrity audit
fe683d95  Complete deep backup persistence audit
d4e194c9  Explain complete backup integrity failures on mobile
c3e2cdab  Explain complete backup integrity failures on desktop
77fd7dd4  Fix MaterialLedger namespace closure
6375d567  Add complete latest handoff snapshot
70b74928  Point new chats to latest complete handoff
```

Проверка:

Actions run `31243187630` действительно был `failure` на commit `78ac24533f1157080bd2163990dbdb0b2577807c`. Полный лог доказал конкретную syntax error в `CM_MaterialLedger.cpp`. Исправление `77fd7dd4` закоммичено, но новый ESP32 Actions run после этого fix в этой записи ещё не подтверждён как GREEN.

Где остановились:

Backend deep backup integrity собран. Точный следующий cleanup — упростить `CM_WindingPersistenceIntegrityAudit`, используя authoritative `WindingJournalQuery::validateAll()` вместо собственного pagination-based полного обхода.

Следующее действие:

1. Fetch актуальный `CM_WindingPersistenceIntegrityAudit.cpp` и перевести его на `validateAll()` без изменения safety semantics.
2. При наличии нового красного ESP32 run — читать полный job log и чинить первую реальную ошибку.
3. Audit backup HTTP/error semantics.
4. Performance/rotation review для растущих NDJSON без преждевременной смены storage model.
5. Реальный hardware E2E ESP32 + Arduino остаётся обязательным внешним этапом.

Полный контекст: `docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md`.

---

## 2026-08-07 16:11 — Обновление документации для переноса

Цель:

Синхронизировать handoff с фактическим состоянием ветки после большой серии изменений ESP32 winding flow.

Что сделано:

- переписан `01_CURRENT_STATE.md` под текущую архитектуру;
- переписана точная точка продолжения `06_ACTIVE_WORK_AND_NEXT_STEPS.md`;
- обновлён индекс ключевых файлов `09_KEY_FILES_INDEX.md`;
- зафиксированы реализованные persistent allocator, snapshot/state recovery, strict linked job, winding history, runtime microSD readiness, shared winding parser и lifecycle UI;
- удалены из активного backlog пункты, которые уже реализованы;
- следующей задачей определён end-to-end путь `client → motor → repair → linked winding → history` через `repairs.html` mobile/desktop.

Последние функциональные коммиты перед handoff:

```text
b33ff222617cce7ed1fd41a069a5bdeb8ff323d8
a3b59cff66d48538cc38087c65c968346c86f54a
4e56ac07590be3eb44665b7a4f1f1c0fa39d5423
0703f828163513007e2694af28467a684d0700b2
f7a16f4a63b8bfef595af0897509b5698bd7b8e4
adede9bd93eabe338f313cd04f90482caee38df2
93180abfc8d4927b4282926b3213e7f7426a92be
90841c8dc43fde2511a5180d528829ca0cc46d55
b57a898d3921ef4c0c7dbf4a17a8e32770abbe4a
```

Документационные коммиты этой точки переноса начинаются с:

```text
7542afb23bf030081ef98292476f8e9ed67561de  current state
aea8b9378c2a0637aaa66a4493c9a96a89ce83fd  next steps
8d9dcb9d9568d8464f0601ab7f4b65ab5dc706b5  key files index
```

Проверка:

Пользователь ранее в этой сессии подтвердил, что проверенные им предыдущие коммиты зелёные. Документация не считается самостоятельным доказательством CI для последующих функциональных коммитов.

Где остановились:

Backend full winding flow и основные safety/integrity слои собраны. Следующий рабочий участок — UI ремонта mobile/desktop и проверка всей цепочки до истории намотки.

Следующее действие:

Перечитать актуальные `firmware/esp32/web/mobile/repairs.html` и `firmware/esp32/web/desktop/repairs.html`, затем пройти и исправить цепочку `клиент → двигатель → ремонт → linked winding → history`.

---

## 2026-08-06 — Усиление журнала намотки

Цель:

Защитить журнал ESP32 от неправильной последовательности событий Arduino.

Что сделано:

- строгая проверка `RUN_STARTED` и `RUN_COMPLETED`;
- запрет завершения без старта;
- проверка совпадения session_id;
- последовательный completed_runs;
- одна активная намотка на сессию;
- запрет повторного и уменьшающегося run_id внутри сессии.

Файлы:

```text
firmware/esp32/src/CM_WindingJournal.h
firmware/esp32/src/CM_WindingJournal.cpp
```

Последние коммиты:

```text
ae58c1908d570f4489a0d58cd9167fd7e6b4c257
5d8dcea4800485f5dcecd8357fa43b96cfed5ff5
```

Проверка:

```text
NOT VERIFIED
```

Где остановились:

Нужно проверить `ESP32 Build` и `CMP Protocol Tests` для текущего head.

Следующее действие:

После зелёной сборки создать `docs/79_MONOTONIC_WINDING_RUN_IDS_PER_SESSION.md`, затем исправить дедупликацию журнала, чтобы ключ включал `session_id`.

---

## 2026-08-06 — Создание постоянного handoff-контекста

Цель:

Сделать возможным переход в новый чат без потери архитектуры, истории и текущей задачи.

Что сделано:

Создан каталог:

```text
docs/PROJECT_HANDOFF/
```

В него добавлены:

- инструкция для нового чата;
- текущее состояние;
- архитектура и подключения;
- UART-протокол;
- данные, API, склад и UI;
- журнал выполненного;
- точная точка продолжения;
- будущие планы;
- правила проверки;
- индекс файлов;
- этот журнал сессий.

Создан корневой указатель:

```text
CONTINUE_CMP_PROTOCOL_V1.md
```

Коммиты создания handoff:

```text
a7eaab0473bb6edf9a41f40643c347526ef6eac8
66b3b609089892f7261f3391a5d7aefa8e103045
ceb8297a5503653319b9d6b99fef4575fedeee7a
6ecc86b23eb1e95859af999ca2483b5708609509
7a238dd6e3562554f50604730779e543f2e06130
15f18cc5184e75c9c4b160615c3d76b067970121
b7e66a04840b63c0246ef65b7aadf2c9dd0d1505
b6efde9e9e3ec7b396660175a600e07ab20c2704
6e3f543bb233e1d24224449fba0298d4cd135f08
d0843662455fbcd76c09e5f481479acb9303ef12
89e9867a97f690bcdb14ae8a129eb075b88637e1
```

Проверка:

Файлы записаны в ветку `cmp-protocol-v1`. Это документационные изменения; проверка кода не заменена и остаётся отдельным следующим шагом.

Где остановились:

Handoff-каталог готов. Активная техническая задача остаётся в `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.
