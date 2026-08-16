# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` —
   самый свежий checkpoint: read-only microSD capacity diagnostics подтверждена
   на реальном ESP32; следующий и оставшийся обязательный gate — final
   populated-device acceptance / recovery drill.
2. `33_MICROSD_CAPACITY_DIAGNOSTICS_2026-08-16.md` — реализация read-only
   capacity/used/free diagnostics без automatic cleanup; ESP32 Build и web audit
   SUCCESS.
3. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import
   успешно выполнен на реальном ESP32, запись сохранилась после reboot.
4. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве;
   добавлен release safety contract audit.
5. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — backend
   fail-closed lock на persisted apply evidence, explicit cleanup exception и
   scheduler wait-state.
6. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активная работа и история продолжения;
   при расхождении использовать checkpoint `34` и текущий код.
7. `01_CURRENT_STATE.md` — общее состояние; предыдущие проценты готовности могут
   быть старее checkpoint `34`.
8. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура и питание.
9. `03_PROTOCOL_AND_WINDING_FLOW.md` — фактический UART/CMP1 flow.
10. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и
    правила изменения/проверки.

Файлы `12`–`33` сохраняют предыдущие checkpoints и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions и подтверждённые hardware
   tests;
3. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md`;
4. checkpoints `33`, `32`, `31`, затем `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и
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

CoilMaster v1 оценивается в **96%**.

На реальном устройстве подтверждены последние отдельные hardware release-gates:

- positive operator-only transactional restore apply;
- motor import через production UI/API с сохранением записи после reboot;
- read-only microSD capacity diagnostics в `Настройки` после актуальной прошивки
  и обновления `/web`.

Для microSD diagnostics repo-level проверки подтверждены:

```text
ESP32 Build 31938372488 — SUCCESS
CMP/web audit 31938393947 — SUCCESS
final handoff-head CMP/web/release-contract 31938462767 — SUCCESS
```

Конкретные числовые capacity/used/free значения в handoff не записаны, потому
что пользователь подтвердил корректную работу целиком без передачи самих чисел.

Закрытые hardware gates не повторять, пока соответствующий production-код не
меняется.

Recovery path остаётся fail-closed: persisted apply evidence блокирует новые
backup/restore действия до explicit cleanup, scheduler ждёт
`WAITING_RESTORE_CLEANUP`, auto-resume отсутствует. В CI действует
`Tests/Web/check_release_contracts.js`, а web audit дополнительно защищает
read-only microSD diagnostics и отсутствие automatic cleanup.

## Следующий обязательный release gate

**Final populated-device acceptance / recovery drill.**

Минимальный scope:

1. обычный reboot и доступность clients / motors / repairs / warehouse /
   winding history;
2. отсутствие automatic physical START и ESP32/Web SSR authority;
3. корректная linked winding история и exact spool/session/run provenance;
4. manual writeoff остаётся manual и не возникает только из `RUN_COMPLETED`;
5. fresh backup создаётся и доступен для inspection/read;
6. reboot не продолжает restore/apply автоматически;
7. network/time/diagnostics/settings UI работают без release-blocking ошибок.

Destructive fault-injection разрешён только на disposable card/image.
