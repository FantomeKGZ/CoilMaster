# CoilMaster — продолжение проекта

Дата обновления: **2026-08-20**

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

## Аппаратный справочник

Для физической сборки, проверки проводов, RTC и Hall использовать:

```text
docs/HARDWARE_REFERENCE/00_READ_FIRST.md
docs/HARDWARE_REFERENCE/01_ARDUINO_CONNECTIONS.md
docs/HARDWARE_REFERENCE/02_ESP32_CONNECTIONS.md
docs/HARDWARE_REFERENCE/03_KEYS_AND_HIDDEN_COMMANDS.md
docs/HARDWARE_REFERENCE/04_RTC_TIME_SYNC.md
docs/HARDWARE_REFERENCE/07_HALL_SENSOR_CALIBRATION.md
```

Hall reference фиксирует factory baseline `A0 / threshold 590 / hysteresis 50`, live-калибровку по реальному ADC и обязательные tests против повторного счёта при зависшем магните.

## Начать отсюда

1. `40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md` — **актуальная точка продолжения**: утверждённая новая модель двигателя/UI, `program + repeat_target`, автоматическое завершение active JOB после последнего подтверждённого RUN, Hall live calibration/settings, redesign Arduino archive, kg-first material plan и общий web UX roadmap.
2. `39_JOB_CANCEL_RECOVERY_2026-08-18.md` — production firmware hardening: resilient `JOB_CANCEL`, Arduino `ALL_CLEAR`, recovery после reboot и hardware regression.
3. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` — исторический release-ready checkpoint до последующих firmware/UI изменений.
4. `37_RELEASE_CANDIDATE_BASELINE_2026-08-16.md` — release-candidate deployment baseline до JOB cancel/recovery hardening.
5. `36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md` — final populated-device acceptance для предыдущего production baseline.
6. `35_FINAL_ACCEPTANCE_CONTRACT_AUDIT_2026-08-16.md` — repo-level final acceptance contract audit и CI protection.
7. `34_MICROSD_DIAGNOSTICS_HARDWARE_PASS_2026-08-16.md` — read-only microSD capacity diagnostics подтверждена на реальном ESP32.
8. `32_MOTOR_IMPORT_HARDWARE_PERSISTENCE_PASS_2026-08-16.md` — motor import успешно выполнен на реальном ESP32, запись сохранилась после reboot.
9. `02_ARCHITECTURE_AND_HARDWARE.md`, `03_PROTOCOL_AND_WINDING_FLOW.md`, а также `../03_ARDUINO_CORE.md`, `../04_ESP32_CORE.md`, `../05_CMP_APPLICATION_PROTOCOL.md`, `../10_DIAGNOSTICS.md` — аппаратная архитектура и UART/CMP flow.
10. `09_KEY_FILES_INDEX.md` и `08_WORK_RULES_AND_VERIFICATION.md` — индекс и правила изменения/проверки.
11. `01_CURRENT_STATE.md` и `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — исторические сводки; при расхождении использовать текущий код и checkpoint `40`.

Предыдущие checkpoints сохраняют историю и не заменяют текущий код. `11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат build/Actions и подтвержденные hardware tests;
3. `40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md` для следующего функционального блока;
4. `39_JOB_CANCEL_RECOVERY_2026-08-18.md` для текущего JOB cancel/recovery поведения;
5. `38_COILMASTER_V1_RELEASE_READY_2026-08-16.md` для предыдущего подтвержденного release baseline;
6. остальные checkpoints и тематические документы.

Перед каждым изменением существующего файла заново получать его содержимое и blob SHA из `cmp-protocol-v1`. Для нового файла сначала проверять отсутствие точного пути. `main` не использовать как источник реализации.

## Safety-инварианты

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- writeoff остается ручным и сохраняет exact run provenance;
- legacy exact spool provenance не уничтожать при будущей kg-first миграции;
- linked immutable snapshot/history не удаляется operational cancellation;
- backup restore operator-only, transactional и fail-closed;
- reboot не продолжает restore/apply автоматически;
- persisted restore evidence блокирует новые backup/restore действия до explicit cleanup;
- заполнение microSD не запускает automatic deletion production data;
- destructive fault injection на рабочей microSD запрещен;
- hardware settings менять только при доказанном safe idle;
- Hall calibration/test endpoint не выполняет START и не включает SSR.

## Утверждённая новая семантика winding program

Не путать программу и количество повторов.

```text
38/38 × 6
```

означает:

```text
program = [38, 38]
repeat_target = 6
```

ESP32 отправляет Arduino **один JOB**. Arduino выполняет программу один раз после physical START, затем предлагает следующий повтор и снова ждёт physical START. Никакого automatic START между повторами.

Различать:

```text
repeat_target     — сколько нужно выполнить в текущем JOB
completed_runs    — сколько фактически выполнено в текущем JOB
historical_runs   — сколько программа выполнялась когда-либо
coil_count        — отдельная физическая характеристика, если нужна
```

После последнего подтверждённого `RUN_COMPLETED` active JOB должен быть безопасно очищен автоматически после persistence/delivery handshake, а история RUN остаётся immutable.

## Новая модель двигателя — обязательные поля

Следующий UI/schema block должен добавить:

```text
manufacturer
model
phase_count
slot_count
program / coil_program
repeat_target
```

`slot_count` отображается и в quick list, и в detail card.
Legacy `name` сохранить для backward compatibility, но не делать главным обязательным операторским полем.

## Hall SS49E — текущая точка

Factory/reference baseline:

```text
pin A0
threshold 590
hysteresis 50
release threshold 540
```

Старый рабочий sketch пользователя давал ориентировочно `522` в покое и `660` с магнитом. Реальный UI должен показывать фактические ADC values конкретной установки.

На реальном стенде обнаружен repeated-count edge case при магните, остающемся над Hall. В код добавлена stable-release защита:

```text
081b3ed1dc3849ec8b0c6898fd841acb6d5f2d76
fix: debounce Hall sensor release before rearming

2b14a91c5a90d8fd2d694c5c54308f47fcfea0b4
fix: require stable Hall release before next turn
```

Следующий шаг — hardware regression и затем persistent/live Hall settings по checkpoint `40` и `docs/HARDWARE_REFERENCE/07_HALL_SENSOR_CALIBRATION.md`.

Не утверждать UNO build/CI green для этого изменения без фактического результата.

## JOB cancel/recovery hardening после release-ready checkpoint 38

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
- ни один recovery path не создаёт `RUN_COMPLETED` и не списывает провод.

Host protocol/web checks и PlatformIO builds Arduino Uno + ESP32 для этого старого change-set прошли в one-shot verifier до final commit `1c66938c...`. Это не является подтверждением более новых Hall/UI changes.

## Hardware status

Предыдущий production baseline был hardware-accepted, но после него менялись JOB recovery, FTP/RTC/firmware identification и Hall logic. Поэтому hardware acceptance не переносить автоматически на текущий HEAD.

Для Hall минимум проверить:

```text
stationary magnet -> максимум 1 count
remove + return -> ровно следующий count
magnet already present at START -> нет бесконечного счёта
noise/vibration around release threshold -> нет серии false counts
real rotating magnet -> нет пропусков нормальных оборотов
```

Для JOB/repeat target будущий acceptance описан в checkpoint `40`.

## Предыдущий CoilMaster v1 baseline

Hardware-accepted ESP32/Web baseline предыдущего состояния:

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

Эти результаты остаются доказательством предыдущего release state, но не заменяют verification текущего HEAD.

## Следующее практическое действие

Следовать `40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md`.

Начать с:

```text
Phase 0 — reconcile current cmp-protocol-v1 HEAD/build/workflows
Phase 1 — Hall correctness + real ADC measurements
Phase 2 — Arduino persistent hardware settings + safe CFG protocol
Phase 3 — desktop/mobile live Hall calibration UI
```

Только после этого переходить к `repeat_target`/automatic final JOB clear и большой переработке motors/Arduino archive.

## Короткий текст для нового чата

```text
Продолжаем CoilMaster.
Repo FantomeKGZ/CoilMaster, source-of-truth branch cmp-protocol-v1, main не использовать.
Сначала прочитай docs/PROJECT_HANDOFF/00_READ_FIRST.md и
40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md.
Также прочитай docs/HARDWARE_REFERENCE/07_HALL_SENSOR_CALIBRATION.md.
Перед изменением существующего файла fetch актуальный blob SHA.
Не утверждай build/CI green без фактической проверки.
Начать с Phase 0/Phase 1: reconcile branch и Hall hardware correctness, затем live calibration/settings.
```
