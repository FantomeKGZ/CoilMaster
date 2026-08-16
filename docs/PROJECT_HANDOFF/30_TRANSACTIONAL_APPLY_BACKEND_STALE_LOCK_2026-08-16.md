# Checkpoint 30 — transactional apply backend stale lock

Дата: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Что закрыто

После transactional restore apply persisted recovery evidence больше нельзя
обойти прямым HTTP-вызовом после reboot.

До этого UI уже fail-closed блокировал новые backup/restore действия при
`GET /api/backup/remote/apply-status -> STALE`, но backend busy-gate
`RemoteBackupWeb::applyActive()` учитывал только runtime apply stages. После
reboot runtime stage снова `Idle`, при этом
`APPLY_JOURNAL.tsv` / `APPLY_RESULT.txt` сохраняются. Поэтому UI был закрыт,
а часть direct HTTP backup/restore действий могла не видеть persisted recovery
state.

Теперь backend `applyActive()` возвращает busy и при наличии любого из:

- `/data/settings/remote-restore-rollback/APPLY_JOURNAL.tsv`;
- `/data/settings/remote-restore-rollback/APPLY_RESULT.txt`.

Это распространяет fail-closed lock на уже существующие action handlers,
которые используют `applyActive()`.

## Explicit cleanup сохранён

`DELETE /api/backup/remote/staging` остаётся единственным операторским путем
очистки stale restore evidence. Для него busy-проверка намеренно использует
только live runtime apply stages, а не persisted-aware `applyActive()`.

Следовательно:

- cleanup запрещён во время реального apply/rollback;
- cleanup всё ещё запрещён при active winding через `BackupActivityGuard`;
- после reboot при `STALE` оператор может выполнить explicit cleanup;
- никакого automatic cleanup / auto-resume не добавлено.

## Apply status

`GET /api/backup/remote/apply-status` разделяет runtime activity и persisted
recovery evidence:

- live apply/rollback -> `active=true`;
- после reboot surviving evidence -> `state=STALE`, `active=false`.

Это не выдаёт stale recovery evidence за продолжающуюся background operation.

## Scheduled backup

Scheduler теперь до записи `m_scheduleAttemptDate=today` проверяет apply
evidence. Если evidence существует, состояние становится:

`WAITING_RESTORE_CLEANUP`

и дневная попытка не расходуется. После explicit cleanup scheduled backup может
снова быть рассмотрен в тот же день при выполнении остальных условий.

## Regression audit

`Tests/Web/check_web_assets.js` теперь статически проверяет четыре контракта:

1. persisted apply journal/result входят в backend `applyActive()` lock;
2. explicit cleanup использует runtime-only apply guard;
3. `STALE` остаётся runtime-inactive в `apply-status`;
4. scheduler имеет `WAITING_RESTORE_CLEANUP` до записи daily attempt.

## Commits

Firmware hardening:

`b81166bb58ebcbaf4a1f5667d2be7905b484d32e`
`Fail closed on persisted restore apply evidence`

Regression audit:

`1f959ee72628076497133e2b8cce50f60070cbb8`
`Audit backend stale restore lock`

## Проверки

Для firmware commit `b81166b...`:

- CMP Protocol Tests + web audit — **SUCCESS**, run `31933792385`;
- ESP32 Build — **SUCCESS**, run `31933792370`.

Для test-only HEAD `1f959ee...`:

- CMP Protocol Tests + web audit — **SUCCESS**, run `31933860297`;
- ESP32 Build не запускался этим commit, поскольку firmware не менялся; его
  parent firmware commit `b81166b...` уже имеет подтверждённый успешный build.

Не подставлять старые RAM/Flash значения: для этого checkpoint они отдельно не
извлекались.

## Safety-инварианты не изменены

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не списывает провод автоматически;
- writeoff ручной и привязан к exact
  `spool_id + source_session_id + source_run_id`;
- restore остаётся operator-only;
- reboot не продолжает apply автоматически.

## Следующий обязательный hardware gate

Готовность проекта остаётся **93%** до фактического positive transactional
apply test на устройстве.

Последовательность:

1. создать свежую V2 backup-копию;
2. `inspection -> staging -> restore plan -> rollback snapshot -> apply preflight`;
3. получить `READY`;
4. при остановленной машине вручную нажать `Применить проверенную копию`;
5. подтвердить действие и вручную повторить exact batch ID;
6. дождаться `APPLIED`, не обесточивая устройство;
7. reboot;
8. подтвердить `STALE`, `active=false`, отсутствие auto-resume и блокировку
   новых backup/restore действий;
9. вручную проверить clients, motors, repairs, warehouse и winding data;
10. выполнить explicit cleanup;
11. убедиться, что после cleanup новый backup/restore flow снова разрешён.

Fault-injection, намеренное повреждение данных и отключение питания во время
apply на production microSD не выполнять. Такие тесты допустимы только на
disposable card/image.
