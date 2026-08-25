# CoilMaster — completion estimate and next-chat transfer

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Repo: `FantomeKGZ/CoilMaster`

Этот checkpoint является текущим authoritative transfer для продолжения проекта в новом чате. Старые numbered checkpoints остаются history/evidence и не являются активным backlog.

## Stable pre-CRM baseline

Перед началом нового Web/CRM этапа `main` был clean fast-forward синхронизирован с текущей стабильной точкой `cmp-protocol-v1`:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
```

До sync `cmp-protocol-v1` был ahead of `main` на 583 commits и behind на 0. Дополнительно создан reference branch:

```text
stable-2026-08-25-pre-crm-redesign
```

Подробности: `docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md`.

После snapshot все дальнейшие изменения выполняются только в `cmp-protocol-v1`; `main` не использовать как source и не двигать до следующего явно согласованного stable checkpoint.

## Current phase

Stage-1 repo-only optimization закрыт. Реальная двухплатная hardware acceptance была начата и выявила operator-B exit defect; исправление прошло Uno/ESP32/CMP verification. Полный hardware acceptance ещё не завершён.

Пользователь после этого утвердил новый большой active product block: **Workshop Web/CRM redesign**.

Authoritative design:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Current queue:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

## Source of truth / working rules

- единственная source-of-truth ветка: **`cmp-protocol-v1`**;
- `main` не использовать как источник кода;
- перед изменением existing file: fetch exact current content + current blob SHA;
- перед созданием new file: проверить exact path и убедиться в 404/not found;
- не объявлять CI/build/hardware GREEN без фактической проверки или явного подтверждения оператора;
- после каждого meaningful implementation block обновлять `docs/PROJECT_HANDOFF` сразу, а не в конце серии.

## Hardware ownership and safety — unchanged

```text
ESP32: service/data/UI orchestration, SD/RTC/network, workshop registry,
       jobs/persistence, warehouse/material/costing, backup/restore,
       Web UI and extended Hall calibration analysis/history

CMP1 UART: commands/jobs/config down; run/status/calibration events up

Arduino Uno: physical START, SSR authority, normal Hall realtime count,
             keypad/LCD/buzzer, local calibration safety gates,
             realtime winding state machine and RUN events
```

Never weaken:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly drives SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts wire/material;
- cancellation/operator abort never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion or NDJSON truncation.

## Approved target Workshop/CRM architecture

```text
CLIENT
  -> linked physical MOTOR(s)
  -> REPAIR
  -> immutable AS_RECEIVED motor/winding snapshot
  -> WORKING / STARTING winding jobs
  -> resulting winding version
  -> COSTING
  -> PAYMENTS / BALANCE
  -> repair completed
  -> DELIVERED_TO_CLIENT
```

### Motors

- `motors.html` becomes catalog-only and adopts the compact Arduino archive layout.
- Create `/desktop/motor-new.html` for motor creation.
- Remove duplicated embedded motor forms from catalog/repair pages; leave links to the new page.
- `motor-details.html` becomes the main motor work card.
- One physical motor remains one `motor_id`.
- Rewinding Al -> Cu or later rewinds create winding versions, not duplicate motors.
- Each winding version supports separate WORKING and STARTING roles.
- 3-phase motors normally expose only WORKING.
- Conductor data must support mixed/multiple wires such as `0.95 + 1.00`, `0.80 x 3`, etc.
- WORKING and STARTING can be sent directly to the machine from the motor card.
- Sending a job never means automatic physical START.

### Clients

- `clients.html` becomes catalog-only.
- Create `/desktop/client-new.html`.
- Create `/desktop/client-details.html`.
- Remove duplicated embedded client creation from repairs; leave links.
- Client card shows linked physical motors, repairs, payments, balance, accepted/completed/delivered dates.
- Do not permanently embed owner identity into motor master record; linkage comes through repair/history semantics so ownership changes remain representable.

### Repair snapshots / delivery

- New repair captures immutable/read-only `as received` motor/winding snapshot.
- Later updates of current motor/winding data must not rewrite old repair history.
- Repair completion/CLOSED and physical delivery to the client are different states/events.
- Add append-only `DELIVERED_TO_CLIENT`-equivalent evidence with `repair_id + client_id + motor_id + delivered_at`.
- Outstanding debt warns the operator but does not permanently block delivery; operator may explicitly confirm delivery in debt.

### Costing vs cash

Existing costing stays responsible for cost, labour, client price and margin/loss.

Add separate append-only payment/cash subsystem:

```text
client_id
repair_id
payment/correction id
amount
timestamp
```

Create `/desktop/cash.html` with charged/paid/balance/status views.

Support:

- full payment;
- partial payment;
- multiple payments;
- debt/overpayment balance;
- append-only correction instead of silent rewrite;
- client aggregated payment history/balance.

## Wire accounting simplification — approved migration, not yet implemented

User approved moving the main workflow away from mandatory exact `spool_id` selection because the current workshop has few spools and exact-spool UX is unnecessarily heavy.

Target future contract:

```text
source_session_id + source_run_id
material class CU/AL
actual consumed weight
manual confirmation
```

The spool inventory UI can remain available as an optional inventory interface.

CRITICAL: current production backend/finalization still uses exact spool identity. Until the coordinated migration is complete, current exact-spool checks remain authoritative. Do **not** only hide/remove spool selector in Web.

The migration must update coherently:

- linked job creation;
- immutable job metadata/snapshot semantics;
- writeoff API/storage;
- costing;
- finalization guards;
- backup whitelist/integrity;
- reports/history;
- Web;
- tests/docs.

Post-migration invariant remains:

```text
RUN_COMPLETED never auto-deducts material.
Actual consumption is still explicit/manual and provenance-bound to the exact run.
```

## Backward compatibility rules

- existing `motors.ndjson` records remain readable;
- legacy `coil_program + repeat_target` can be synthesized as a legacy/current WORKING version;
- do not destructively rewrite historical motors just to adopt versioning;
- existing repairs/history remain readable;
- prefer append-only sidecar/version/event stores for new history;
- every new production store must enter backup whitelist + integrity audit before release-critical use;
- restore must validate new stores before they are required;
- desktop may lead the UX implementation, but mobile must share the same data semantics.

## Implementation order

```text
A. schema/contracts + migration contracts
B. motor-new + motors catalog redesign + motor-details/winding versions
C. client-new + clients catalog + client-details
D. repair as-received snapshot + delivery lifecycle
E. exact-spool -> material+actual-weight coordinated migration
F. append-only cash/payments + cash.html
G. navigation/mobile alignment + regressions + backup/restore integrity
H. repeat full hardware E2E acceptance on the final contracts
```

Detailed numbered steps are in checkpoint 95 and `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

## Hardware acceptance status

Not complete.

Current real-device evidence from this session:

- ESP32 firmware built/flashed and initialized its services;
- Uno firmware built/flashed and reached normal UI;
- recovered ESP32 job did not auto-resume after reboot;
- real winding operation exposed B-exit behavior defect;
- B/operator-abort code correction subsequently passed Uno/ESP32/CMP verification.

Because the approved redesign will change linked winding/writeoff/finalization contracts, a complete E2E hardware pass must be performed again after those changes stabilize.

## Documentation discipline for current phase

Keep synchronized after every meaningful block:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
this file
docs/PROJECT_HANDOFF/00_READ_FIRST.md
```

Create new numbered checkpoints for major persistence/API subsystems and record exact commit SHA + CI runs + migration/compatibility status.

## Read order for a new chat

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/93_STAGE1_SOFTWARE_OPTIMIZATION_CLOSURE_2026-08-25.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

## Ready-to-paste prompt for the next chat

```text
Продолжаем проект CoilMaster.

Репозиторий: FantomeKGZ/CoilMaster.
Единственная source-of-truth ветка: cmp-protocol-v1. main для исходников не использовать.

Перед CRM redesign зафиксирована stable pre-CRM точка 449570d47649d5f6336a31ee3eed491256e0fb1a: main и stable-2026-08-25-pre-crm-redesign указывают на этот commit. Дальнейшая разработка только в cmp-protocol-v1; main не двигать до следующего согласованного stable checkpoint.

Сначала прочитай /AGENTS.md, docs/PROJECT_HANDOFF/00_READ_FIRST.md, docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md, docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md, docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md и docs/PROJECT_HANDOFF/01_CURRENT_STATE.md.

Stage-1 optimization закрыт. Hardware acceptance был начат, но не завершён; найденный operator-B defect исправлен и software verification прошёл. Текущий активный этап — Web/CRM redesign из checkpoint 95.

Начинай с Phase A schema/contracts, не с косметического HTML. Сохрани backward compatibility существующих motors/repairs. Один физический motor_id должен иметь versioned winding history с WORKING/STARTING. Создание motor/client переносится на отдельные страницы; создаются client-details и cash/payment subsystem; repair CLOSED отделяется от delivered-to-client.

Переход от exact spool_id к material class + actual manual weight одобрен, но ещё не реализован. Не удаляй spool requirement частично: миграция должна одновременно обновить job/writeoff/costing/finalization/backup/integrity/reports/Web/tests. RUN_COMPLETED никогда не списывает материал автоматически. Physical START остаётся только локальным, SSR остаётся Uno-only.

После каждого meaningful block своевременно обновляй 95, 06, 01, 90 и при необходимости 00; для крупных новых stores/API создавай новый numbered checkpoint с commit/CI evidence.
```
