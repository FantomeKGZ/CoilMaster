# CoilMaster — positive restore apply hardware PASS + release contracts

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Hardware gate закрыт

Пользователь подтвердил на реальном устройстве positive transactional restore
apply: проверка завершена успешно, замечаний по результату нет. Повторять этот
gate не требуется.

Это закрывает обязательную аппаратную проверку operator-only transactional
restore apply, которая оставалась открытой в checkpoint `30`.

Не делать из этого подтверждения более широких выводов: destructive
fault-injection по-прежнему не выполнять на рабочей карте. Такие проверки
разрешены только на disposable card/image.

## Release safety contracts

После hardware PASS добавлен автоматический repo-level audit:

- `Tests/Web/check_release_contracts.js`;
- commit `7b385fd7d3ace6e3cce2c122b0889fedd8da42e0`.

Audit защищает ключевые production-инварианты:

- physical START остаётся физической кнопкой Arduino;
- SSR authority остаётся на Arduino; ESP32 source не получает `SsrController`
  или `Pins::Ssr`;
- boot Arduino возвращает state machine в HOME;
- ESP32 продолжает публиковать `automatic_queue_allowed=false`,
  `automatic_resume_allowed=false` и `automatic_wire_writeoff_allowed=false`;
- manual wire writeoff требует exact `spool_id + source_session_id +
  source_run_id`, сверяет immutable spool selection и completed source run и
  блокирует повторное списание того же source run;
- restore apply сохраняет explicit `confirmed=APPLY`, persisted journal/result
  lock, runtime/stale distinction, `WAITING_RESTORE_CLEANUP` и `auto_resume=0`;
- executable motor-import audit остаётся обязательным для desktop и mobile.

Workflow `.github/workflows/cmp-protocol-tests.yml` расширен commit
`6baf9071c425082cb5201e7191a4f242ad0baaf4`: release-contract audit запускается
в стандартном host job, а PR path coverage теперь включает `Tests/Web`,
`Arduino`, `Core`, `firmware/arduino`, `firmware/esp32/src` и
`firmware/esp32/web`.

## Проверка

GitHub Actions run `31934159579` для commit `6baf9071...` завершён
**SUCCESS**. В одном job успешно прошли:

- CMP protocol configure/build/tests;
- существующий web JavaScript/navigation/import audit;
- новый release safety contract audit.

Firmware в этом блоке не изменялся, поэтому новый hardware flash для самого
release-contract audit не требуется.

## Готовность

После подтверждённого positive restore apply аппаратный recovery gate закрыт.
Текущая оценка CoilMaster v1: **94%**.

Оставшиеся release gates:

1. **Motor import hardware acceptance** — выполнить импорт на реальном ESP32,
   убедиться, что запись доступна в базе после импорта и сохраняется после
   reboot. Host-side desktop/mobile importer audit уже проходит.
2. **Final populated-device acceptance / recovery drill** — пройти основной
   production flow на заполненной тестовыми данными системе и проверить
   reboot/recovery границы без destructive fault injection на рабочей карте.
3. Дальнейшие robustness/performance улучшения не должны задерживать v1, если
   они не выявляют конкретный release blocker. Заполнение microSD не должно
   приводить к автоматическому удалению production данных.

## Safety-инварианты без изменений

- automatic physical START запрещён;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot запрещён;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и связан с exact
  `spool_id + source_session_id + source_run_id`;
- backup restore остаётся operator-only и fail-closed;
- никакой destructive recovery/fault-injection на рабочей microSD.
