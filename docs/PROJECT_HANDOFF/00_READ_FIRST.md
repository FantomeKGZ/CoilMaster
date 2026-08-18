# CoilMaster — продолжение проекта

Дата обновления: **2026-08-18**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Для AI / coding agent

Перед поиском по всему репозиторию сначала использовать maintenance-layer:

```text
/AGENTS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/01_PROJECT_MAP.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Он содержит карту ownership/composition roots, маршрут `что изменить → какие файлы открыть`, правила добавления нового модуля и change-scoped verification matrix. Тематические и исторические handoff-файлы читать после того, как эта карта сузила нужный subsystem.

## Начать отсюда

1. `39_JOB_CANCEL_RECOVERY_2026-08-18.md` — **актуальный checkpoint после production firmware hardening**: устранение зависшего JOB, resilient `JOB_CANCEL`, Arduino `ALL_CLEAR`, recovery после reboot и обязательный hardware regression после перепрошивки обеих плат.
2. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` — исторический release-ready checkpoint до изменения production firmware 2026-08-18.
3. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md` — release-candidate deployment baseline до JOB cancel/recovery hardening.
4. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md` — final populated-device acceptance / recovery drill = HARDWARE PASS для предыдущего production baseline.
5. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md` — repo-level final acceptance contract audit и CI protection.
6. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` — read-only microSD capacity diagnostics подтверждена на реальном ESP32.
7. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import успешно выполнен на реальном ESP32, запись сохранилась после reboot.
8. `31_POSITIVE_RESTORE_APPLY_HARDWARE_PASS_AND_RELEASE_CONTRACTS_2026-08-16.md` — positive transactional restore apply подтвержден на реальном устройстве.
9. `02_ARCHITECTURE_AND_HARDWARE.md`, `03_PROTOCOL_AND_WINDING_FLOW.md`, а также `../03_ARDUINO_CORE.md`, `../04_ESP32_CORE.md`, `../05_CMP_APPLICATION_PROTOCOL.md`, `../10_DIAGNOSTICS.md` — аппаратная архитектура, фактический UART/CMP1 flow и новый cancel/recovery behavior.
10. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и правила изменения/проверки.
11. `01_CURRENT_STATE.md` и `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — исторические сводки; при расхождении использовать текущий код и наиболее новый numbered checkpoint.

Предыдущие checkpoints сохраняют историю и не заменяют текущий код. `11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат build/Actions и подтвержденные hardware tests;
3. `39_JOB_CANCEL_RECOVERY_2026-08-18.md` для текущего JOB delivery/cancel/recovery состояния;
4. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` для подтвержденного предыдущего release baseline;
5. остальные checkpoints и тематические документы.

Перед каждым изменением существующего файла заново получать его содержимое и blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие точного пути. `main` не использовать как источник реализации.

## Safety-инварианты

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- writeoff остается ручным и требует exact `spool_id + source_session_id + source_run_id`;
- linked immutable snapshot/spool history не удаляется operational cancellation;
- backup restore operator-only, transactional и fail-closed;
- reboot не продолжает restore/apply автоматически;
- persisted restore evidence блокирует новые backup/restore действия до explicit cleanup;
- заполнение microSD не запускает automatic deletion production data;
- destructive fault injection на рабочей microSD запрещен.

## Важное изменение после release-ready checkpoint 38

После подтвержденного CoilMaster v1 release baseline обнаружен edge case рассинхронизации задания ESP32 ↔ Arduino. Production firmware был изменен для устранения проблемы.

Ключевые commits:

```text
7d8bc93fdcd626f358dd1baa22428b8447355b2d
fix: make ESP32 Arduino job cancellation resilient

1c66938cd52ce790b9833faf93fc647b5bae5725
fix: allow safe cancellation of linked no-run jobs
```

Теперь:

- pending JOB после возможной отправки отменяется через remote `JOB_CANCEL`, а не локальным discard;
- accepted linked no-run job можно безопасно отменить до physical START;
- Arduino cancellation идемпотентна для already-clear state;
- физический fallback `D → * → # → D` отправляет CRC-protected `ALL_CLEAR`;
- ESP32 умеет коррелировать `ALL_CLEAR` с recovered persisted job после reboot;
- ни один recovery path не создает `RUN_COMPLETED` и не списывает провод.

Host protocol/web checks и PlatformIO builds Arduino Uno + ESP32 для этого change-set прошли в fail-fast one-shot verifier до final commit `1c66938c...`.

## Hardware status после изменения 2026-08-18

Предыдущий production baseline был hardware-accepted, но новый JOB cancel/recovery код изменяет production firmware. Поэтому новый behavior требует targeted hardware regression после прошивки **обеих** плат из одной актуальной точки `cmp-protocol-v1`.

Минимум проверить:

```text
JOB → JOB_ACK ACCEPTED
lost ACK → cancel
accepted no-run → cancel
repeated cancel after already clear
ESP32 reboot with persisted no-run uncertainty
Arduino D → * → # → D → ALL_CLEAR
emergency clear blocked during active/paused winding
no automatic wire writeoff
```

До прохождения этих hardware checks не переносить формулировку «100% hardware accepted» со старого baseline на новый cancel/recovery change-set.

## Предыдущий CoilMaster v1 baseline

Checkpoint `38` остается исторически подтвержденным: до изменения JOB recovery пользователь после полного release-candidate цикла подтвердил, что production flow работает.

Hardware-accepted ESP32/Web baseline того состояния:

```text
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
```

Confirmed ESP32 Build того baseline:

```text
ESP32 Build run 31938372488 — SUCCESS
```

Confirmed release-candidate CI:

```text
CMP Protocol Tests run 31941111206 — SUCCESS
head: df188ca49d95ee4953bd228c05aec849dcd947b5
```

Эти результаты остаются доказательством предыдущего release state, но не заменяют targeted hardware regression нового firmware change.

## Следующее практическое действие

Прошить Arduino Uno и ESP32 текущей версией `cmp-protocol-v1` и выполнить hardware regression из checkpoint `39`. После успешной проверки зафиксировать новый hardware-accepted checkpoint и только тогда обновлять release baseline.
