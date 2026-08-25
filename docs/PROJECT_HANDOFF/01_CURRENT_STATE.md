# Текущее состояние CoilMaster

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает только текущее состояние. История и доказательства находятся в numbered checkpoints.

## Source of truth

Единственный источник реализации — `cmp-protocol-v1`. `main` для исходников не использовать.
Перед изменением existing file получать current content + blob SHA. Для нового path сначала проверять отсутствие файла. Не объявлять CI/build/hardware GREEN без фактической проверки.

## Stable snapshot before CRM redesign

Перед новым Web/CRM этапом stable point зафиксирован на:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
```

`main` fast-forward синхронизирован с этой точкой. Дополнительно создан reference branch:

```text
stable-2026-08-25-pre-crm-redesign
```

Подробности: `docs/PROJECT_HANDOFF/96_STABLE_MAIN_SNAPSHOT_BEFORE_CRM_2026-08-25.md`.

После snapshot все новые изменения снова идут только в `cmp-protocol-v1`; `main` сохраняется как pre-CRM stable baseline до следующего явно согласованного stable checkpoint.

## Current project state

Stage-1 repo-only performance optimization закрыт. Финальная двухплатная hardware acceptance была начата на реальном ESP32 + Arduino Uno, выявила operator-B exit defect и привела к исправлению operator abort behavior. Полный hardware gate ещё не завершён.

После этого пользователь утвердил новый крупный product/Web этап: **Workshop Web/CRM redesign**.

Authoritative design:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Active queue:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

## Production architecture unchanged at hardware boundary

```text
ESP32:
  Wi-Fi/AP/HTTP/FTP, SD/RTC, workshop registry, jobs/persistence,
  warehouse/material/costing, backup/restore, Web UI,
  extended Hall calibration analysis/history

CMP1 UART:
  JOB/control/config down
  run/status/calibration events up

Arduino Uno:
  physical START, SSR authority, normal Hall realtime count,
  keypad/LCD/buzzer, calibration local safety gates,
  realtime winding state machine, RUN_STARTED/RUN_COMPLETED
```

Production protocol remains text `CMP1|...`.

## Approved new Workshop/CRM direction

Target domain flow:

```text
CLIENT
  -> physical MOTOR
  -> REPAIR
  -> immutable AS_RECEIVED winding snapshot
  -> WORKING / STARTING winding jobs
  -> resulting winding version
  -> COSTING
  -> PAYMENTS / BALANCE
  -> repair completion
  -> DELIVERED_TO_CLIENT
```

Key decisions:

- `motors.html` becomes catalog only and adopts Arduino archive style/layout;
- motor creation moves to `/desktop/motor-new.html`;
- `motor-details.html` becomes the main motor work card;
- one physical motor keeps one `motor_id` while winding changes become version history;
- each winding version supports WORKING and STARTING roles;
- 3-phase motors normally show only WORKING;
- conductor model must support combinations such as `0.95 + 1.00` and `0.80 x 3`;
- direct send of WORKING/STARTING from motor card is required, but physical START remains local-only;
- `clients.html` becomes catalog only;
- client creation moves to `/desktop/client-new.html`;
- add `/desktop/client-details.html` with motors, repairs, payments, balance and delivery dates;
- remove duplicated inline client/motor creation forms from repair page and replace with links;
- closing a repair and physically delivering the motor are separate events;
- cash/payment subsystem becomes separate from costing;
- add append-only payments/corrections and `/desktop/cash.html`;
- payment does not hard-block delivery; debt creates warning + explicit operator confirmation.

## Motor/winding history rule

One physical motor must not be duplicated merely because it was rewound from Al to Cu.

Target model:

```text
motor_id
  winding version 1: AS_RECEIVED / ORIGINAL
  winding version 2: REWOUND / CURRENT
  winding version N: later repair
```

Existing legacy motor records with `coil_program + repeat_target` remain readable during migration and can be synthesized as a legacy WORKING version until upgraded.

## Wire accounting migration decision

User approved simplifying the main workflow away from mandatory exact spool selection.

Target future manual consumption contract:

```text
source_session_id + source_run_id
material class CU/AL
actual consumed weight
manual confirmation
```

However **this migration is NOT implemented yet**. Current production backend/finalization still uses exact `spool_id`, so existing spool safety checks must remain until the whole chain is migrated coherently.

Do not partially remove spool UI/backend requirements before job creation, writeoff, costing, finalization, backup/integrity, reports and regression contracts are all updated together.

## Safety boundary — unchanged

Never weaken:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never auto-writes off material;
- cancellation/operator abort never erases immutable run/history evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/NDJSON truncation.

Until wire-accounting migration is complete, linked-production writeoff still follows current exact-spool implementation.

## Hardware acceptance status

Not complete.

Observed during current real-device pass:

- ESP32 production firmware built, flashed and booted normally;
- Uno production firmware built, flashed and reached normal UI;
- reboot did not automatically resume recovered job state;
- keypad/UI operation exposed the B-exit defect during winding;
- B-exit/operator-abort software correction passed Uno/ESP32/CMP build verification.

A complete E2E hardware pass remains required after contract-changing Web/CRM/wire changes are stabilized.

## NDJSON/storage rule

- no premature DB migration;
- no destructive migration of historical motor/client/repair records;
- prefer append-only sidecar/version/event stores for new history;
- no automatic cleanup/rotation;
- every new production store must enter backup whitelist + integrity validation before becoming release-critical.

## Documentation rule for the new phase

Documentation is updated with each meaningful implementation block, not at the end.

Required living files:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
```

For major new persistence/API subsystems, add new numbered checkpoints with exact commits and CI evidence.
