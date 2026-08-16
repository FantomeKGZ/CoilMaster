# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` —
   самый свежий checkpoint: motor import успешно выполнен на реальном ESP32,
   запись сохранилась после reboot; следующий gate — final populated-device
   acceptance / recovery drill.
2. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве;
   добавлен автоматический release safety contract audit.
3. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — backend
   fail-closed lock на persisted apply evidence, explicit cleanup exception и
   scheduler wait-state.
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активная работа и история продолжения;
   при расхождении с новым checkpoint использовать checkpoint `32` и текущий код.
5. `01_CURRENT_STATE.md` — общее состояние; предыдущие проценты готовности могут
   быть старее checkpoint `32`.
6. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура и питание.
7. `03_PROTOCOL_AND_WINDING_FLOW.md` — фактический UART/CMP1 flow.
8. `09_KEY_FILES_INDEX.md` — индекс production-файлов.
9. `08_WORK_RULES_AND_VERIFICATION.md` — правила изменения и проверки.

Файлы `12`–`31` сохраняют предыдущие checkpoints и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions и подтверждённые hardware
   tests;
3. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md`;
4. checkpoint `31`, затем `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и
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
- fail-closed semantics не ослаблять ради UI convenience.

## Текущая точка

CoilMaster v1 оценивается в **95%**.

На реальном устройстве теперь подтверждены два последних отдельных release-gate:

- positive operator-only transactional restore apply;
- motor import через production UI/API с сохранением импортированной записи
  после reboot.

Motor-import hardware gate закрыт и не требует повторения, пока соответствующий
production-код не меняется.

Recovery path остаётся fail-closed: persisted apply evidence блокирует новые
backup/restore действия до explicit cleanup, scheduler ждёт
`WAITING_RESTORE_CLEANUP`, auto-resume отсутствует. В CI действует
`Tests/Web/check_release_contracts.js`, который защищает physical START/Arduino
SSR authority, no-auto-resume/writeoff, exact manual writeoff linkage,
transactional restore lock и executable desktop/mobile motor-import audits.
Последний зафиксированный release-contract CI в checkpoint `31` завершён
SUCCESS; документационные commits checkpoint `32` не считать новым firmware
build.

Следующий обязательный release gate — **final populated-device acceptance /
recovery drill**. Он должен проверять уже собранные подсистемы как единый
эксплуатационный набор и не требует повторять без причины уже закрытые hardware
gates. Destructive fault-injection разрешён только на disposable card/image.
