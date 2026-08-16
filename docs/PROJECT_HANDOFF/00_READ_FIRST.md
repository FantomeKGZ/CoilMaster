# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   самый свежий checkpoint: positive transactional restore apply подтверждён на
   реальном устройстве; добавлен автоматический release safety contract audit.
2. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — backend
   fail-closed lock на persisted apply evidence, explicit cleanup exception и
   scheduler wait-state.
3. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активная работа и точное продолжение,
   включая завершённый общий CRC рабочего CMP1.
4. `01_CURRENT_STATE.md` — текущее состояние и предыдущая оценка готовности;
   при расхождении использовать checkpoint `31`.
5. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура и питание.
6. `03_PROTOCOL_AND_WINDING_FLOW.md` — фактический UART/CMP1 flow.
7. `09_KEY_FILES_INDEX.md` — индекс production-файлов.
8. `08_WORK_RULES_AND_VERIFICATION.md` — правила изменения и проверки.

Файлы `12`–`30` сохраняют предыдущие checkpoints и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions и подтверждённые hardware
   tests;
3. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md`;
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `01_CURRENT_STATE.md`;
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

CoilMaster v1 оценивается в **94%**. Stable external-power Wi-Fi, read-only
restore preflight и **positive operator-only transactional restore apply** теперь
подтверждены на реальном устройстве. Positive apply hardware gate закрыт и не
требует повторения.

Recovery path остаётся fail-closed: persisted apply evidence блокирует новые
backup/restore действия до explicit cleanup, scheduler ждёт
`WAITING_RESTORE_CLEANUP`, auto-resume отсутствует. В CI добавлен
`Tests/Web/check_release_contracts.js`, который защищает physical START/Arduino
SSR authority, no-auto-resume/writeoff, exact manual writeoff linkage,
transactional restore lock и наличие executable desktop/mobile motor-import
audits. GitHub Actions run `31934159579` завершён SUCCESS.

Следующий обязательный hardware-gate — **motor import hardware acceptance**:
импортировать запись на реальном ESP32, убедиться, что она появилась в базе и
сохранилась после reboot. После него — final populated-device acceptance /
recovery drill. Destructive fault-injection разрешён только на disposable
card/image.
