# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md` —
   release-candidate deployment baseline: точный ESP32/Web production state,
   реально прошедший final hardware acceptance, и граница между production и
   последующими test/docs-only commits.
2. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md` —
   **final populated-device acceptance / recovery drill = HARDWARE PASS**;
   последний обязательный hardware release gate закрыт.
3. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md` — единый repo-level
   final acceptance contract audit и CI protection.
4. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` — read-only microSD
   capacity diagnostics подтверждена на реальном ESP32.
5. `33_MICROSD_CAPACITY_DIAGNOSTICS_2026-08-16.md` — реализация read-only
   capacity/used/free diagnostics без automatic cleanup.
6. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import
   успешно выполнен на реальном ESP32, запись сохранилась после reboot.
7. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` —
   positive transactional restore apply подтверждён на реальном устройстве.
8. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `01_CURRENT_STATE.md` — исторические
   сводки; при расхождении использовать checkpoints `37`/`36` и текущий код.
9. `02_ARCHITECTURE_AND_HARDWARE.md` и `03_PROTOCOL_AND_WINDING_FLOW.md` —
   аппаратная архитектура и фактический UART/CMP1 flow.
10. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и
    правила изменения/проверки.

Предыдущие checkpoints сохраняют историю и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат build/Actions и подтверждённые hardware tests;
3. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md`;
4. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md`;
5. checkpoints `35`, `34`, `33`, `32`, `31`;
6. остальные handoff и тематические документы.

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
- backup restore operator-only, transactional и fail-closed;
- reboot не продолжает restore/apply автоматически;
- persisted restore evidence блокирует новые backup/restore действия до explicit
  cleanup;
- заполнение microSD не запускает automatic deletion production data;
- destructive fault injection на рабочей microSD запрещён.

## Текущая точка

CoilMaster v1 оценивается в **98%**.

**Все обязательные production hardware acceptance gates закрыты.**

На реальном устройстве подтверждены, среди прочего:

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

## Release-candidate production baseline

Реально проверенный ESP32/Web production baseline:

```text
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
```

Подтверждённый build:

```text
ESP32 Build run 31938372488 — SUCCESS
```

После этого production baseline до checkpoint `36` менялись только tests,
workflow и handoff docs; production firmware/web paths не менялись.

Final repo-level acceptance protection:

```text
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Подтверждённый final CI перед hardware PASS:

```text
CMP Protocol Tests run 31940069683 — SUCCESS
```

## Что осталось до final v1 closure

Это уже не повторный production E2E. Оставшиеся небольшие release-closure задачи:

1. точный Arduino flashed revision зафиксировать при следующей плановой прошивке,
   если требуется формальный deployment baseline; не перепрошивать только ради
   номера SHA;
2. отдельно проверить `http://coil.local/`, если mDNS должен считаться
   обязательным convenience release requirement; IP fallback остаётся рабочим;
3. destructive corruption/power-loss/fault-injection проводить только на
   disposable microSD/image как отдельный hardening;
4. после выбранных closure-пунктов оформить final CoilMaster v1 release status.

До **100%** не повышать только на основании документации: 100% означает также
закрытую release packaging/closure и отсутствие известных release-blocking
пунктов.
