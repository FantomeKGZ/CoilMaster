# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` —
   **финальный статус CoilMaster v1: RELEASE READY, 100% согласованного production scope**.
2. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md` —
   release-candidate deployment baseline: точный ESP32/Web production state,
   реально прошедший final hardware acceptance, и граница между production и
   последующими test/docs-only commits.
3. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md` —
   final populated-device acceptance / recovery drill = HARDWARE PASS.
4. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md` — единый repo-level
   final acceptance contract audit и CI protection.
5. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` — read-only microSD
   capacity diagnostics подтверждена на реальном ESP32.
6. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import
   успешно выполнен на реальном ESP32, запись сохранилась после reboot.
7. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве.
8. `02_ARCHITECTURE_AND_HARDWARE.md` и `03_PROTOCOL_AND_WINDING_FLOW.md` —
   аппаратная архитектура и фактический UART/CMP1 flow.
9. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и
   правила изменения/проверки.
10. `01_CURRENT_STATE.md` и `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — исторические
    сводки; при расхождении использовать checkpoint `38` и текущий код.

Предыдущие checkpoints сохраняют историю и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат build/Actions и подтверждённые hardware tests;
3. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md`;
4. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md`;
5. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md`;
6. остальные checkpoints и тематические документы.

Перед каждым изменением существующего файла заново получать его содержимое и
blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие
точного пути. `main` не использовать как источник реализации.

## Safety-инварианты релиза

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- writeoff остаётся ручным и требует exact
  `spool_id + source_session_id + source_run_id`;
- backup restore operator-only, transactional и fail-closed;
- reboot не продолжает restore/apply автоматически;
- persisted restore evidence блокирует новые backup/restore действия до explicit
  cleanup;
- заполнение microSD не запускает automatic deletion production data;
- destructive fault injection на рабочей microSD запрещён.

## Финальная точка CoilMaster v1

**CoilMaster v1 = RELEASE READY. Готовность: 100% согласованного production scope.**

Пользователь после полного release-candidate цикла подтвердил: **«все работает отлично»**.

Все обязательные production hardware acceptance gates закрыты. На реальном
устройстве подтверждены, среди прочего:

- полный linked production flow с exact spool и physical START;
- RUN_STARTED / RUN_COMPLETED;
- manual exact-run wire writeoff;
- costing/finalization/CLOSED;
- backup и backup-while-active negative gate;
- positive operator-only transactional restore apply;
- motor import + persistence after reboot;
- read-only microSD capacity diagnostics;
- final populated-device reboot/data/recovery/backup/network/time/diagnostics
  acceptance без release-blocking ошибок.

Закрытые hardware gates не повторять, пока соответствующий production-код не
меняется.

## Release production baseline

Реально проверенный ESP32/Web production baseline:

```text
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
```

Подтверждённый build:

```text
ESP32 Build run 31938372488 — SUCCESS
```

После этого production firmware/web paths не менялись; release closure меняла
только tests/workflow/handoff docs.

Final repo-level acceptance protection:

```text
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Подтверждённый release-candidate CI:

```text
CMP Protocol Tests run 31941111206 — SUCCESS
head: df188ca49d95ee4953bd228c05aec849dcd947b5
```

## После v1

Следующие действия не блокируют текущий release-ready статус и относятся к
последующему hardening/maintenance:

- destructive corruption / power-loss fault injection — только на disposable
  microSD/image;
- exact Arduino flashed commit можно формально записать при следующей плановой
  прошивке, без перепрошивки только ради SHA;
- `http://coil.local/` можно отдельно проверить как convenience-функцию; IP
  fallback остаётся обязательным operational access path.

Любое будущее изменение production-кода должно проходить релевантные ему build,
CI и hardware regression gates перед включением в новый release baseline.
