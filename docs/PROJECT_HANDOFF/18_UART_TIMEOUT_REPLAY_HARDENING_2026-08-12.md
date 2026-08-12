# UART timeout / replay hardening — 2026-08-12

Ветка: `cmp-protocol-v1`

## Что закрыто

### 1. JOB delivery timeout больше не считается безопасным terminal state

Раньше после исчерпания повторных отправок `JOB` без `JOB_ACK` ESP32 сохранял `TIMED_OUT`, но runtime и post-reboot recovery могли разрешить новое задание.

Это было слишком оптимистично: Arduino могла принять `JOB`, а все подтверждения могли потеряться.

Теперь `TIMED_OUT` трактуется как неоднозначное состояние и требует operator/manual review:

```text
JOB retries exhausted
→ persist delivery_state=TIMED_OUT
→ JobRecovery::ManualReviewRequired
→ new_job_allowed=false
→ backup blocked
→ no automatic resend
→ no automatic resume
```

Явный `REJECTED` остаётся безопасным terminal delivery state, потому что это положительно полученный ответ Arduino.

### 2. Operator review поддерживает TIMED_OUT

`JobStateStore::closeAfterManualReview()` принимает `TIMED_OUT` как состояние, которое оператор может закрыть только после явной проверки физического состояния станка.

После `CLOSED_AFTER_REVIEW` новое задание снова может быть разрешено. Safety-инварианты не менялись: physical START остаётся физическим, ESP32 не управляет SSR напрямую, auto-resume отсутствует.

### 3. Backup fail-closed согласован с timeout recovery

Fallback `BackupActivityGuard` считает `TIMED_OUT` busy до operator review.

`CLOSED_AFTER_REVIEW` имеет приоритет над историческим delivery state и считается terminal/safe для backup.

### 4. Локально сгенерированный timeout больше не означает `arduino_online=true`

`lastArduinoEventMs` обновляется только после реально принятого Arduino ACK/event.

Локальные `JOB`/`JOB_CANCEL` timeout больше не могут искусственно показывать Arduino online в `/api/status`.

### 5. Linked journal replay hardening

Предыдущий блок уже изменил duplicate semantics linked journal: повтор `(session_id, run_id, event_type)` считается `Duplicate` только при совпадении `completed_runs`; конфликтующий replay получает `InvalidTransition` и не может изменить persisted runtime state.

### 6. Autonomous Arduino archive replay hardening

Для `LOCAL_EVT` duplicate теперь требует полного semantic match:

```text
session_id
run_id
event_type
completed_runs
job_type
coil_count
program
```

Если identity совпадает, но payload отличается, событие возвращается как `Invalid`, а ESP32 отправляет `INVALID_LOCAL_EVENT`, а не `DUPLICATE`.

Повторный exact-identical frame остаётся идемпотентным.

## Основные commits этого блока

```text
1ce6a17ee08444278e262ec4785e3e83c4ce7f3e  Treat job delivery timeout as manual review
85674874edb75e26e126d0a4c738f7fa226b33e1  Keep timed out jobs under manual review
57e42285b9919768df0eaec3303344eae42cef0d  Block backup after ambiguous job timeout
96740c896e7242f29769fa517deef241aeedb657  Require manual review after job ACK timeout
390b8ed99b1be11fd5d41f2552be32a7d3c32ec4  Honor reviewed closure in backup fallback
894978bdc0f3aadebe8dacb9c8f3015c5dfbd396  Add exact autonomous event replay check
64e7c77e99ea8d66729f505738c2ca2080140fff  Reject conflicting autonomous event replays
97795673cae3db5bfec46a363e89fca5c470c9ca  Do not report UART timeout as Arduino online
```

## Verification status

Предыдущий ESP32 build до этого блока был подтверждён пользователем как успешный.

Для текущего HEAD после UART timeout/replay изменений:

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

Нужен новый clean ESP32 build.

## Безопасный hardware negative test после успешной сборки

Тест timeout проводить без работающего двигателя.

1. Убедиться, что Arduino физически не выполняет намотку.
2. Создать сервисное (unlinked) задание при недоступном UART ACK/отключённой Arduino.
3. Дождаться исчерпания retry.
4. Проверить `/api/status`:

```text
job_status=MANUAL_REVIEW_REQUIRED
manual_review_required=true
new_job_allowed=false
job_creation_ready=false
arduino_online=false
```

5. Проверить `/api/backup/manifest`:

```text
export_allowed=false
snapshot_stability_checked=false
```

6. После физической проверки станка выполнить существующий operator recovery/acknowledge.
7. Убедиться, что persisted state стал `CLOSED_AFTER_REVIEW` и новое задание снова разрешено.

Не проводить timeout test на работающем двигателе и не использовать его как способ аварийной остановки.
