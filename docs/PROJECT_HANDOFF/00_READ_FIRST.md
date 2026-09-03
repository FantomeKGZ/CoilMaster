# CoilMaster — current project entrypoint

Дата обновления: **2026-09-03**  
Repo: `FantomeKGZ/CoilMaster`  
Рабочая/source-of-truth ветка: **`cmp-protocol-v1`**.  
Stable/ready ветка: **`main`**.

## Branch policy

Текущая схема веток:

```text
cmp-protocol-v1 = единственная активная development/source ветка
main            = stable/ready, переносить только подтверждённые checkpoints
```

`arduino-ru-lcd-experiment` больше не является рабочей веткой и не использовать её как source.

Перед изменением существующего файла обязательно получить exact current content из `cmp-protocol-v1` и текущий blob SHA. Для нового файла сначала подтвердить отсутствие пути. Не считать новый HEAD GREEN без exact `completed/success` применимого CI.

## Current work stream — feature completeness

Идёт полный аудит ранее обсуждавшихся обновлений/добавлений функций. Цель: проверять реальную реализацию, заканчивать только proven incomplete items и не переоткрывать закрытые блоки без concrete regression.

Previous checkpoints 159–167, motor questionnaire/edit/import, client questionnaire, bounded clients/motors/repairs pagination, Web stale Statistics cleanup и Web regression reachability остаются закрытыми, если нет concrete regression. Speculative repeated-scan/performance и Uno micro-optimization без измеримой необходимости не возобновлять.

## Current feature block — new motor captures canonical WORKING / STARTING winding

Блок создания/редактирования/просмотра обмотки двигателя закрыт через checkpoint 18.

Ключевой контракт сохраняется:

```text
GET  /api/motors/winding/latest?motor_id=...
POST /api/motors/winding/role
optimistic token = expected_winding_version_id
stale token      = HTTP 409 winding_version_conflict
storage          = append-only motor-winding-versions.ndjson
```

При создании новой карточки:

- `WORKING` обязателен;
- `STARTING` создаётся только при включённом `Есть пусковая обмотка`;
- материал задаётся независимо для каждой роли: `CU` / Медь или `AL` / Алюминий;
- для каждой роли принимается 1–5 физических проводов;
- пользовательские разделители диаметров: `;` или `:`, можно смешивать;
- десятичный знак: запятая или точка;
- одинаковые диаметры разрешены и canonical-нормализуются через `xN`;
- legacy `coil_program` / `repeat_target` берутся из WORKING;
- первая versioned запись всегда WORKING; STARTING использует возвращённый version id и не может стать orphan first version.

Canonical conductor model остаётся единым: `CU/AL:<hundredths_mm>xN`, без второго wire-storage формата. `MotorWindingRoleSpec::MaxConductors = 5U`.

Desktop/mobile `motor-edit.html` показывают friendly `Материал + Провод`, но сохраняют raw canonical fallback для legacy/смешанных записей. `motor-details.html` показывает материал и диаметры человекочитаемо и сохраняет raw fallback для неизвестного формата.

Detailed record:
```text
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
```

Exact implementation GREEN:
```text
bb6af586d51ace86a584ec3f24e5a96b7d4e9e0d
CMP Protocol Tests #4770 run 33708384422 / SUCCESS
```

## Bounded clients / motors / repairs pagination audit — NO-CHANGE

Current desktop/mobile catalog pages already use bounded cursor pagination consistently:

```text
clients.html  -> GET /api/clients?limit=20&cursor=...
motors.html   -> GET /api/motors?limit=20&cursor=...
repairs.html  -> GET /api/repairs?limit=20&cursor=...
```

All six desktop/mobile pages maintain previous-cursor history, consume `has_more` / `next_cursor`, reset paging when the search/filter changes, and reject non-monotonic returned cursors.

Backend `RepairRegistryWeb` defaults to page size 20, validates cursor/limit, caps page size at `RepairRegistry::MaxListPageSize = 32`, delegates to bounded page serializers and returns `count`, `limit`, `cursor`, `has_more`, `next_cursor`, `max_page_size`.

Result: the requested large-list pagination for clients, motors and repairs is already implemented. No code change is justified unless a concrete regression appears.

## Web completeness / parity audit — current closed checkpoints

Current main Web audit now makes functional regression contracts reachable instead of allowing silent orphan tests.

Covered by `Tests/Web/check_web_assets.js` and the CI graph:

- client creation required/optional fields and desktop/mobile parity;
- FTP `/web` recovery/safe-idle/local-subnet/atomic upload contracts;
- remote backup/restore and retention UI parity;
- repair material/writeoff/correction/exact RUN_WIRE parity;
- CRUD page separation and internal route validity;
- dashboard Arduino job history remains read-only and does not gain SSR/automatic START coupling;
- client CRM/cash/prepayment/history contracts;
- Cash UI bounded append-only payment behavior;
- RU Hall local-control/SSR/Web safety regression.

Checkpoint 21 additionally enforces that every `Tests/Web/check_*.js` file is reachable from `cmp-protocol-tests.yml` directly or transitively through local `require()` links.

Exact recent GREEN:
```text
29f4749ba917446b88ea625fc31e811baa849a93
CMP Protocol Tests #4825 run 33728574447 / SUCCESS

a3c13cec8d9049c4a35cdd218e11ace6b50e4f04
CMP Protocol Tests #4826 run 33728672746 / SUCCESS
```

Detailed record:
```text
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
```

The historical empty `desktop/mobile statistics.html` placeholders remain absent and are not to be restored without a real feature requirement.

## Uno resource checkpoint

Latest measured production Uno code checkpoint before documentation-only commits:

```text
Flash: 31114 / 32256; 1142 bytes free
RAM:   1227 / 2048; 821 bytes free
```

The agreed 512-byte RAM/Flash CI margin remains mandatory. Do not lower it to mask code growth.

## Current audit direction

The previous items `stale Statistics page` and `orphaned Web regression contracts` are closed.

Continue only with a concrete unresolved functional/runtime defect found on current `cmp-protocol-v1`, or an explicit user-requested feature. For each proven gap:

1. fetch exact current file content from `cmp-protocol-v1` + blob SHA;
2. make the minimal behavior-preserving fix;
3. add/update regression coverage where applicable;
4. verify exact applicable CI for the new HEAD;
5. update HANDOFF meaningfully.

Do not use the deferred backlog as an automatic execution queue.

## Physical / prior engineering state

Physical Arduino+ESP32 E2E for the previously tested hardware/firmware state remains operator-confirmed PASS. Hardware verification must be repeated only when a current change actually affects the relevant physical path.

## Safety invariants

Do not change:

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically writes off wire;
- manual linked RUN_WIRE requires exact `spool_id + source_session_id + source_run_id`;
- restore/recovery remain fail-closed/operator-controlled;
- mutation-time authoritative rereads/TOCTOU guards remain;
- append-only evidence is not silently edited/deleted;
- no automatic production truncation/rotation/deletion;
- no premature DB/index migration.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/21_CHECKPOINT_WEB_REGRESSION_REACHABILITY_2026-09-03.md
docs/PROJECT_HANDOFF/20_CHECKPOINT_BRANCH_POLICY_AND_UNO_HEADROOM_2026-09-03.md
docs/PROJECT_HANDOFF/18_CHECKPOINT_MOTOR_NEW_WINDING_CAPTURE_2026-09-03.md
docs/PROJECT_HANDOFF/17_CHECKPOINT_WORKING_STARTING_EDIT_2026-08-31.md
docs/PROJECT_HANDOFF/07_REPAIR_MATERIAL_WRITEOFF_PLAN.md
docs/PROJECT_HANDOFF/13_HALL_RU_LCD_ACCEPTANCE.md
docs/71_PRICING_HISTORY_CURRENT_INVARIANTS.md
```

## Immediate NEXT

Audit current `cmp-protocol-v1` for a real unresolved user-facing/runtime defect or previously promised feature that is still incomplete. Change only proven gaps; keep `main` stable until an explicitly promoted verified checkpoint.
