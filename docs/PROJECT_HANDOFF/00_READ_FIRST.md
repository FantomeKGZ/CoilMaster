# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md` —
   самый свежий checkpoint: добавлен единый repo-level final acceptance contract
   audit; CI подтверждён SUCCESS. Остался реальный final populated-device
   acceptance / recovery drill.
2. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` — read-only microSD
   capacity diagnostics подтверждена на реальном ESP32.
3. `33_MICROSD_CAPACITY_DIAGNOSTICS_2026-08-16.md` — реализация read-only
   capacity/used/free diagnostics без automatic cleanup; ESP32 Build и web audit
   SUCCESS.
4. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import
   успешно выполнен на реальном ESP32, запись сохранилась после reboot.
5. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве;
   добавлен release safety contract audit.
6. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — backend
   fail-closed lock на persisted apply evidence, explicit cleanup exception и
   scheduler wait-state.
7. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — история активной работы; при расхождении
   использовать checkpoint `35` и текущий код.
8. `01_CURRENT_STATE.md` — общее состояние; предыдущие проценты готовности могут
   быть старее checkpoint `35`.
9. `02_ARCHITECTURE_AND_HARDWARE.md` и `03_PROTOCOL_AND_WINDING_FLOW.md` —
   аппаратная архитектура и фактический UART/CMP1 flow.
10. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и
    правила изменения/проверки.

Предыдущие checkpoints сохраняют историю и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions и подтверждённые hardware
   tests;
3. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md`;
4. checkpoints `34`, `33`, `32`, `31`, затем `06_ACTIVE_WORK_AND_NEXT_STEPS.md`
   и `01_CURRENT_STATE.md`;
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

Repo-level final acceptance hardening теперь включает:

```text
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Новый final acceptance audit проверяет bounded/exact workshop reads, warehouse,
exact ACTIVE spool linkage, status/diagnostics/storage/network/time, backup
inspection, fail-closed restore, manual exact-run writeoff и наличие основных
operator UI страниц desktop/mobile.

Подтверждённый CI:

```text
CMP Protocol Tests run 31940030107 — SUCCESS
```

В этом run успешно прошли protocol tests, web audit, release safety contracts и
final acceptance contracts. Production firmware этим audit-блоком не менялся,
поэтому новая прошивка только ради checkpoint `35` не требуется.

Закрытые hardware gates не повторять, пока соответствующий production-код не
меняется.

## Последний обязательный release gate

**Final populated-device acceptance / recovery drill.**

На текущем устройстве с уже заполненными тестовыми данными:

1. выполнить обычный reboot;
2. проверить доступность clients / motors / repairs / warehouse / winding history;
3. убедиться, что нет automatic physical START и SSR не активируется от Web/ESP32;
4. проверить существующую linked winding историю и корректность
   repair/motor/spool/session/run связей;
5. убедиться, что `RUN_COMPLETED` сам по себе не создал wire writeoff;
6. создать fresh backup и убедиться, что batch завершён и доступен для
   inspection/read;
7. выполнить обычный reboot и убедиться, что restore/apply не продолжается
   автоматически;
8. проверить network/time/diagnostics/settings без release-blocking ошибок.

Destructive fault-injection, intentional corruption и power-loss apply tests на
рабочей microSD запрещены; только disposable card/image.
