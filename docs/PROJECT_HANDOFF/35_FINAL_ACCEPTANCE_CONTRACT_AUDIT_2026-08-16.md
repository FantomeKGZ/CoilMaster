# CoilMaster — final acceptance contract audit

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Контекст

После hardware PASS read-only microSD capacity diagnostics проект дошёл до последнего обязательного release gate: final populated-device acceptance / recovery drill.

Перед аппаратным drill добавлен единый repo-level audit эксплуатационных контрактов, чтобы финальная проверка не зависела только от ручного просмотра отдельных исходников.

## Реализовано

Добавлен:

```text
Tests/Web/check_final_acceptance_contracts.js
```

Commit:

```text
dbef25a5bf81430594fb1febfbe8392f7f6f42b3
```

Audit проверяет:

- bounded list API для clients / motors / repairs;
- exact lookup API `/api/clients/by-id`, `/api/motors/by-id`, `/api/repairs/by-id`;
- доступность warehouse summary;
- обязательный exact `spool_id` и ACTIVE spool identity для linked winding;
- `/api/status`, `/api/system/diagnostics`, `/api/system/time`;
- `/api/system/network`;
- read-only `/api/system/storage`, capacity/used/free и `automatic_cleanup_allowed=false`;
- отсутствие write/remove/rename path в storage diagnostics;
- remote backup batch status + inspection/inspection-status;
- explicit restore `APPLY`, `WAITING_RESTORE_CLEANUP` и `auto_resume=0`;
- manual wire writeoff exact provenance `spool_id + source_session_id + source_run_id`;
- duplicate source-run writeoff guard;
- наличие основных final-acceptance UI страниц desktop/mobile;
- сохранение существующего release safety audit как authoritative guard physical START/Arduino SSR authority.

## CI

Workflow `.github/workflows/cmp-protocol-tests.yml` обновлён commit:

```text
1c3e004560fc004dfa20804ecfdd5444c8914047
```

Добавлен шаг:

```text
Audit final acceptance contracts
node Tests/Web/check_final_acceptance_contracts.js
```

GitHub Actions run:

```text
31940030107
```

завершён **SUCCESS**. Успешно прошли:

- protocol configure/build/tests;
- web JavaScript/navigation/import/storage audit;
- release safety contract audit;
- final acceptance contract audit.

Production firmware этим блоком не изменялся. Поэтому отдельный ESP32 Build или hardware flash только из-за этого audit не требуется.

## Готовность

Текущая оценка CoilMaster v1 остаётся **96%**.

Repo-level final acceptance contracts подтверждены, но оценку не повышать до завершения реального final populated-device acceptance / recovery drill.

## Последний обязательный hardware gate

На текущем устройстве с уже заполненными тестовыми production-данными выполнить безопасный final drill:

1. обычный reboot;
2. открыть clients / motors / repairs / warehouse / winding history и убедиться, что данные доступны;
3. убедиться, что после reboot нет automatic START и SSR не активируется от Web/ESP32;
4. открыть существующую linked winding историю и проверить, что видны корректные repair/motor/spool/session/run связи;
5. убедиться, что сохранённый `RUN_COMPLETED` сам по себе не создал wire writeoff; списание остаётся только ранее выполненной/явной manual operation;
6. создать fresh backup и убедиться, что batch завершён и доступен для inspection/read;
7. выполнить обычный reboot после этого и убедиться, что никакой restore/apply не продолжается автоматически;
8. проверить network/time/diagnostics/settings без release-blocking ошибок.

Уже закрытые positive restore apply, motor import persistence и microSD diagnostics gates отдельно не повторять, если соответствующий production-код не менялся.

Destructive fault injection, intentional corruption и power-loss apply tests на рабочей microSD запрещены; только disposable card/image.

## Safety-инварианты без изменений

- automatic physical START запрещён;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot запрещён;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся manual и exact `spool_id + source_session_id + source_run_id`;
- backup restore остаётся operator-only и fail-closed;
- automatic deletion production data отсутствует;
- destructive fault injection на рабочей microSD запрещён.
