# CoilMaster — repeat target and JOB lifecycle implementation checkpoint

Дата: **2026-08-20**  
Ветка: **`cmp-protocol-v1`**  
Статус: **repeat-target runtime реализован; host regression подтверждён; PlatformIO/hardware E2E ещё требуют подтверждения**

## Source of truth

Использовать только `FantomeKGZ/CoilMaster`, ветку `cmp-protocol-v1`. `main` не использовать как источник реализации.

## Safety-инварианты

Этот блок не меняет следующие правила:

- физический RUN начинается только после физического START;
- ESP32/Web не управляет SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` сам по себе не выполняет writeoff провода;
- промежуточный или ошибочный ACK не может очистить активный JOB;
- история RUN остаётся immutable.

## Зафиксированная семантика программы

Пример:

```text
38/38 × 6
```

означает:

```text
program = [38, 38]
repeat_target = 6
```

Это **один JOB**.

Один полный проход `38/38` является **одним RUN**. Оба сегмента программы используют один и тот же `run_id`.
Новый `run_id` создаётся только для следующего полного повтора программы.

Для каждого из шести повторов требуется физический START. Никакого automatic START между повторами нет.

`completedRuns` увеличивается только после полного завершения всей программы `38/38`.

## Реализованные изменения

### ESP32 repeat-target runtime

Commit:

```text
d7a96cb71b0de72007ecacc5ae89936a0c3db62d
feat: enforce repeat target across ESP32 job runtime
```

Основные изменения:

- web/API JOB принимает `repeat_target`;
- runtime ESP32 хранит `activeRepeatTarget`;
- status JSON возвращает `repeat_target`;
- recovery восстанавливает repeat target из persisted JOB display state;
- `RUN_COMPLETED 1/6 ... 5/6` не считается финальным завершением JOB;
- после промежуточного completion ESP32 остаётся в состоянии ожидания следующего физического повтора;
- создание нового JOB разрешается только после финального repeat target;
- backup/runtime activity остаётся busy, пока активный JOB не достиг repeat target.

Дополнительный compile-safety lifecycle fix:

```text
7334068...
```

восстановил реализации `CM_JobStateStore::closeAfterRemoteCancel()` и `dismissInactive()`.

### Arduino/Core: один run_id на всю программу

Commit:

```text
1ab2671b20ab026e5e4faae87311cc7fff310ed3
fix: keep one run id across winding program segments
```

До исправления переход после первого сегмента `38` через `CoilComplete` возвращал машину в `Ready`, после чего `beginRun()` выделял новый `run_id`. Это ошибочно делило `38/38` на два RUN.

Теперь:

- первый physical START создаёт `run_id` и публикует один `RUN_STARTED`;
- завершение первого `38` переводит машину в `CoilComplete`;
- следующий physical START/continue переводит машину обратно в `Winding`, но **не вызывает `beginRun()`**;
- второй сегмент сохраняет тот же `currentRunId`;
- второй `RUN_STARTED` не публикуется;
- после второго `38` публикуется `RUN_COMPLETED` с тем же `run_id`;
- только следующий repeat из `JobComplete` вызывает `beginRun()` и получает новый `run_id`.

Это сохраняет physical START requirement и не вводит automatic motion.

## Final JOB auto-clear

В Arduino firmware delivery result path для `RUN_COMPLETED` и при `ACK`, и при `DUPLICATE` выполняется:

```text
machine.acknowledgeDeliveredRun(delivery.runId)
```

`acknowledgeDeliveredRun()` очищает remote JOB только если одновременно выполняются условия:

- `runId != 0`;
- source == `Esp32Web`;
- machine state == `JobComplete`;
- status == `Completed`;
- ACK относится к exact `currentRunId`;
- `repeatTargetReached()` == true.

Следовательно:

- ACK `RUN_STARTED` JOB не очищает;
- ACK промежуточного `RUN_COMPLETED 1/6 ... 5/6` JOB не очищает;
- ACK/DUPLICATE exact финального `RUN_COMPLETED 6/6` очищает JOB;
- Arduino возвращается Home автоматически после подтверждения доставки финального completion;
- manual clear после нормального финального JOB больше не должен требоваться.

Диагностические сообщения Arduino:

```text
CM_JOB AUTO_CLEAR result=FINAL_RUN_ACKED
CM_JOB AUTO_CLEAR result=FINAL_RUN_DUPLICATE
```

## Regression test

Создан тест:

```text
Tests/Protocol/test_repeat_target.cpp
```

Commit:

```text
a616ccf867d883493c348db2932185ee327f0121
test: cover repeat target run identity
```

Подключён в штатный CMake/CTest:

```text
4918e826566dcbc528ae4524df77195e096b38d5
test: add repeat target state machine coverage
```

Тест проверяет remote JOB `38/38 × 6`:

1. один JOB;
2. каждый repeat начинается отдельным `startOrResume()`;
3. repeat получает один новый non-zero `run_id`;
4. первый и второй сегмент `38` используют один `run_id`;
5. между сегментами нет второго `RUN_STARTED`;
6. `RUN_COMPLETED` использует тот же run id;
7. `completedRuns` считает полные program cycles;
8. ACK repeat 1..5 не очищает JOB;
9. 7-й repeat блокируется;
10. wrong final run ACK не очищает JOB;
11. exact final ACK очищает remote JOB и возвращает Arduino Home.

## Фактически выполненная host-проверка

Из текущих Core-файлов был собран минимальный host executable командой эквивалентной:

```text
g++ -std=c++14 -Wall -Wextra -Wpedantic -Werror \
    -I. CM_StateMachine.cpp test_repeat_target.cpp -o repeat_test
./repeat_test
```

Результат:

```text
Repeat target state-machine tests passed.
```

То есть StateMachine regression для `38/38 × 6` фактически компилируется и проходит с warnings-as-errors.

## Что пока НЕ подтверждено

Не считать этот блок hardware-verified до выполнения следующего:

- полный repo CMake/CTest;
- `pio run -e uno`;
- `pio run -e esp32`;
- реальный ESP32 ↔ Arduino UART test;
- физический test шести повторов.

GitHub combined status для текущего checkpoint commit не показывал status contexts. Build container также не смог clone GitHub из-за отсутствия DNS/network access. Поэтому green CI/PlatformIO не заявляется.

## Hardware E2E checklist

После прошивки актуальных ESP32 и Arduino:

1. отправить JOB `program=38/38`, `repeat_target=6`;
2. убедиться, что Arduino принял один JOB;
3. нажать physical START — начинается repeat 1;
4. после первого 38 должен быть переход между сегментами без нового `RUN_STARTED`;
5. второй 38 должен использовать тот же `run_id`;
6. после второго 38 должен прийти `RUN_COMPLETED completed=1`;
7. JOB должен остаться активным и ждать следующий physical START;
8. repeat 2 должен получить новый `run_id`;
9. повторить до `6/6`;
10. промежуточные ACK не должны очищать JOB;
11. после exact ACK/DUPLICATE финального `6/6` Arduino должен вывести `CM_JOB AUTO_CLEAR ...` и уйти Home;
12. physical START после финального target не должен запускать 7-й repeat;
13. reboot, например, после `3/6` не должен auto-resume; recovery должен оставаться fail-safe/manual-review.

## Следующий repo-reviewable шаг

После этого checkpoint продолжать основной план из `40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md`.

Приоритет после repeat lifecycle:

1. вывести `repeat_target` и `completed/target` в desktop/mobile JOB UI;
2. затем перейти к motor schema (`manufacturer`, `model`, `phase_count`, `slot_count`, structured program, repeat target);
3. hardware E2E repeat test остаётся обязательным перед окончательным закрытием firmware блока.
