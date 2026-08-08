# Где остановились и что делать дальше

Дата обновления: 2026-08-08 22:22 +06  
Ветка: `cmp-protocol-v1`

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

Самый свежий pause-snapshot:

```text
docs/PROJECT_HANDOFF/13_PAUSE_HANDOFF_2026-08-08_2222.md
```

Полный предыдущий snapshot:

```text
docs/PROJECT_HANDOFF/12_LATEST_HANDOFF_2026-08-08.md
```

## Текущая оценка готовности

Ориентировочно:

- реализованная функциональность production flow: **около 96%**;
- repository/firmware readiness: **около 93%**;
- общая эксплуатационная готовность: **около 90%**.

Эксплуатационная оценка намеренно ниже: для текущего head нет подтверждённого GREEN Actions result, полный ESP32 + Arduino hardware E2E ещё не выполнен, Stage 0 benchmark на реальном dataset ещё не снят.

## Последний кодовый checkpoint перед паузой

```text
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
```

После него pause/handoff commits могут сдвигать HEAD только документацией. При следующем продолжении сначала проверить фактический HEAD ветки.

## Production flow

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual exact-run wire writeoff
→ materials/pricing → finalization preflight
→ CLOSED → archive/report → read-only backup
```

## Safety invariants — не менять

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- automatic resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не выполняет wire writeoff;
- wire writeoff остаётся ручным;
- **новый** writeoff требует exact `spool_id + source_session_id + source_run_id`;
- corrupted persistence/storage loss блокирует опасную операцию fail-closed.

## Последний завершённый correctness-блок — exact-run writeoff provenance

Storage boundary:

```text
c7335631c660a7b5ee71da880a1f77e4e5faa83f  Require exact run provenance for new wire writeoffs
```

`WarehouseStore::confirmSpoolWriteOff()` больше не принимает новую операцию с `source_session_id=0` или `source_run_id=0`.

HTTP boundary:

```text
7b92010294342d2a9cc9a153f306673f5c66ffb9  Require run provenance in wire writeoff API
```

`POST /api/warehouse/write-offs` требует оба exact ID. Отсутствие provenance возвращает `400 source_session_and_run_required`.

UI boundary:

```text
ef0e64838ebb3f0519f6bfe756ade599a07450b9  Require exact run provenance in writeoff UI
```

Shared mobile/desktop helper:

```text
firmware/esp32/web/shared/writeoff-spool-suggestion.js
```

теперь:

- читает paginated winding history;
- ищет незакрытый `RUN_COMPLETED`;
- сверяет immutable spool-selection;
- разрешает POST только с exact session/run и именно immutable spool;
- после подтверждения ищет следующий незакрытый completed run;
- не выполняет automatic writeoff.

Legacy session-only writeoff остаётся только grandfathered/read-only compatibility старых данных. Новые session-only записи запрещены.

## Production bootstrap — актуальное состояние

Реально зарегистрированы:

```text
workshop clients/motors/repairs
motor similarity
warehouse summary/spools/price
warehouse writeoff GET/POST
materials CRUD/usage/adjustments
repair costing/pricing history
conductor calculator/settings
backup manifest/file/session export
winding history
```

Ключевые commits:

```text
961229e410b91b788984a2ee14a1a8b4f1276073  Register warehouse writeoff routes in production bootstrap
74f5bea9273ab47d5df1c4ef2f0557445b21a6ed  Register production material and conductor APIs
75207162c7164121252057391230077d5340be3a  Remove duplicate winding history bootstrap
```

`/api/winding-history` должен регистрироваться только через `StaticSiteServer::begin()`. Не добавлять второй handler в `WarehouseWeb::begin()`.

## Warehouse repair lookup

Tri-state overload уже восстановлен и strict:

```text
WarehouseStore::repairExists(uint32_t repairId, bool& found) const
```

Он находится в:

```text
firmware/esp32/src/CM_WarehouseRepairValidation.cpp
```

Bool overload делегирует tri-state scan. Не считать этот пункт незавершённым.

## Winding / finalization integrity

Authoritative checks:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Закрыто:

- strict flat JSON validation journal records;
- duplicate semantic key rejection;
- startup journal strict validation в существующем pass;
- finalization использует authoritative EOF validation, не cursor-pages;
- manual writeoff completion proof требует full schema + transitions.

Ключевые commits:

```text
f3e02dadce34c7a85e11d23154652a469d6aa111
d92b6f1c19a5d0c5c2abb3663410e9202bcd385e
9e18cd076b8e1ad904e3893aab20cde1a4e596a8
d7c601fe18d8b60794334cb2572e40a7777cc0d2
```

## Flat persisted JSON hardening

Shared validator:

```text
firmware/esp32/src/CM_FlatJsonObjectValidator.h
```

Основные workshop/material/warehouse/settings/winding runtime/deep/history readers усилены fail-closed без нового telemetry-only filesystem pass.

Крупный `CM_MaterialLedger.cpp` уже закрыт:

```text
c4ff5ff4d195959c3fc70bd8e0b8bf72ac5a0525  Fail closed on malformed material ledger runtime state
```

## Conductor settings

Production source:

```text
/data/settings/conductor.json
```

Experimental `/data/settings/conductor-calculator.ndjson` не использовать как production source.

Backup whitelist и deep audit уже выровнены на `conductor.json`; `allow_mixed_diameters` сохраняется и валидируется.

## Stage 0 backup observability

Текущих **29 metrics** достаточно. Не расширять до benchmark.

На стенде снять:

```text
items[].size_bytes
snapshot_stability_duration_ms
9 per-domain *_duration_ms
allocator high-water
material/business/winding/warehouse record counts
winding session file counts + total bytes
```

До измерений не вводить arbitrary rotation threshold, persistent optimistic cache или database migration.

## CI / compile verification

Для текущего code checkpoint GREEN не подтверждён. Connector не показывает общий push-run список; пустые combined statuses/PR workflow runs не означают успешную сборку.

Историческая compile failure `CM_MaterialLedger.cpp: expected '}' at end of input` исправлена commit:

```text
77fd7dd4db3767c33106d63e6f9174e6559b9bc8
```

## Точное следующее repo-only действие после паузы

1. Проверить фактический HEAD `cmp-protocol-v1` и доступный Actions/build status.
2. Проверить compile/link availability:

```text
WarehouseStore::confirmedWriteOffForSourceRun(...)
WarehouseStore::confirmedWriteOffForSourceSession(...)
```

Искать только через точный branch path/history или фактическую сборку. **Не создавать второй symbol по догадке.**
3. Если symbols определены и build чистый — не продолжать бесконечный parser-hardening; перейти к hardware E2E и benchmark.
4. `WireWriteOffCoverageAudit` и другие repeated NDJSON scans рефакторить только после измерений либо отдельной correctness-причины.

## Обязательный внешний этап — hardware E2E

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

Fault cases:

- reboot/manual-review;
- microSD loss;
- corrupted persistence;
- UART timeout/reject/duplicate events;
- duplicate writeoff;
- missing/wrong session/run/spool;
- close without wire coverage;
- backup during active winding.

После этого можно существенно поднимать эксплуатационную готовность выше текущих ~90%.
