# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `33_MICROSD_CAPACITY_DIAGNOSTICS_2026-08-16.md` —
   самый свежий checkpoint: добавлена read-only диагностика физического объёма,
   used/total/free microSD без automatic cleanup; ESP32 Build и web audit SUCCESS.
2. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` —
   motor import успешно выполнен на реальном ESP32, запись сохранилась после
   reboot; hardware gate закрыт.
3. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве;
   добавлен автоматический release safety contract audit.
4. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — backend
   fail-closed lock на persisted apply evidence, explicit cleanup exception и
   scheduler wait-state.
5. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активная работа и история продолжения;
   при расхождении с новым checkpoint использовать checkpoint `33` и текущий код.
6. `01_CURRENT_STATE.md` — общее состояние; предыдущие проценты готовности могут
   быть старее checkpoint `33`.
7. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура и питание.
8. `03_PROTOCOL_AND_WINDING_FLOW.md` — фактический UART/CMP1 flow.
9. `09_KEY_FILES_INDEX.md` — индекс production-файлов.
10. `08_WORK_RULES_AND_VERIFICATION.md` — правила изменения и проверки.

Файлы `12`–`32` сохраняют предыдущие checkpoints и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions и подтверждённые hardware
   tests;
3. `33_MICROSD_CAPACITY_DIAGNOSTICS_2026-08-16.md`;
4. checkpoint `32`, затем `31`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и
   `01_CURRENT_STATE.md`;
5. остальные handoff и тематические документы.

Перед каждым изменением существующего файла заново получать его содержимое и
blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие
точного пути. `main` не использовать как источник реализации.

## Safety-инварианты

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- writeoff остаётся ручным и требует exact
  `spool_id + source_session_id + source_run_id`;
- восстановление backup не выполняется автоматически и не продолжается после
  reboot;
- заполнение microSD не запускает automatic deletion production data;
- fail-closed semantics не ослаблять ради UI convenience.

## Текущая точка

CoilMaster v1 оценивается в **95%**.

На реальном устройстве подтверждены последние отдельные hardware release-gates:

- positive operator-only transactional restore apply;
- motor import через production UI/API с сохранением импортированной записи
  после reboot.

После них добавлена read-only microSD capacity diagnostics:

```text
GET /api/system/storage
```

Settings UI теперь показывает состояние microSD, физический размер карты,
used/total и свободное место. Endpoint явно публикует
`automatic_cleanup_allowed=false`; diagnostics module не имеет delete/rename/
write/append path. Firmware-bearing state подтверждён ESP32 Build run
`31938372488` SUCCESS, final code/test state подтверждён CMP/web run
`31938393947` SUCCESS.

Motor-import и positive restore hardware gates не требуют повторения, пока их
production-код не меняется.

Recovery path остаётся fail-closed: persisted apply evidence блокирует новые
backup/restore действия до explicit cleanup, scheduler ждёт
`WAITING_RESTORE_CLEANUP`, auto-resume отсутствует. В CI действует
`Tests/Web/check_release_contracts.js`, а web audit дополнительно защищает
read-only microSD diagnostics.

Следующий обязательный release gate — **final populated-device acceptance /
recovery drill**. Перед полным acceptance после прошивки текущего firmware и
обновления `/web` проверить новые microSD показатели в `Настройки`. Destructive
fault-injection разрешён только на disposable card/image.
