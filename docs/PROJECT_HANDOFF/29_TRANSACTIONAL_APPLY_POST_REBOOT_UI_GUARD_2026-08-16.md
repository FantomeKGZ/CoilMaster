# CoilMaster handoff — transactional apply post-reboot UI guard

Дата: **2026-08-16**  
Ветка: **`cmp-protocol-v1`**  
Оценка CoilMaster v1: **93%**

## Контекст

Operator-only transactional restore apply уже реализован и остаётся следующим
hardware-gate. Перед positive test проведён короткий repo-review его поведения
после reboot.

Backend intentionally не выполняет auto-resume. После reboot in-memory apply
state и счётчики обнуляются, а surviving `APPLY_JOURNAL.tsv` и/или
`APPLY_RESULT.txt` дают `/api/backup/remote/apply-status` состояние `STALE`.
Поэтому значения runtime counters после reboot нельзя использовать как
доказательство того, изменялись ли рабочие файлы до перезапуска.

## Добавленный fail-closed UI guard

Commits:

```text
327542efa0300a1de093bb6539da82b8e1c37d0a  add stale restore guard
261b9e733bb2619b3cba0a090d10681e8ff6d2c6  desktop backup integration
a7ec95426e6b5797cba80381332da34780a697a8  mobile backup integration
```

Новый файл:

```text
firmware/esp32/web/shared/backup-restore-stale-guard.js
```

Guard работает только через read-only
`GET /api/backup/remote/apply-status`.

Если после reboot сервер сообщает `STALE`, mobile и desktop теперь:

- не трактуют сброшенные runtime counters как подтверждение неизменности данных;
- явно сообщают, что состояние рабочих файлов нужно проверить вручную;
- блокируют запуск новой backup/inspection/staging/plan/rollback/preflight/apply
  операции;
- блокируют single-file remote upload controls;
- оставляют доступным только explicit cleanup временной/rollback области;
- не запускают restore, rollback, START, SSR или wire writeoff.

Это UI hardening. Он не меняет transactional apply C++ write-path и не добавляет
никакого auto-resume после reboot.

## Проверка

Diff от предыдущего handoff HEAD `f2ef159c60bf1bc03dfd2e68a9b3941f00b84d77`
до `a7ec95426e6b5797cba80381332da34780a697a8` содержит только:

```text
firmware/esp32/web/desktop/backup.html
firmware/esp32/web/mobile/backup.html
firmware/esp32/web/shared/backup-restore-stale-guard.js
```

`CMP Protocol Tests` run `31932880386`: **SUCCESS**. Этот workflow включает
web asset/navigation audit и компиляцию shared JavaScript.

`ESP32 Build` run `31932880376` на момент записи этого checkpoint всё ещё
**IN_PROGRESS** на шаге PlatformIO build. Не считать его успешным до отдельного
подтверждения conclusion.

## Следующий hardware-gate

Positive transactional apply test остаётся тем же:

1. обновить current ESP32 firmware и полностью заменить microSD `/web`;
2. использовать свежую V2-копию и пройти
   inspection → staging → plan → rollback → preflight `READY`;
3. на остановленном станке запустить `Применить проверенную копию`, подтвердить
   действие и вручную повторить exact batch ID;
4. дождаться `APPLIED`, не отключая питание;
5. выполнить reboot;
6. после reboot UI должен показывать fail-closed `STALE` warning и не разрешать
   новый restore cycle до проверки/cleanup;
7. вручную проверить clients, motors, repairs, warehouse и winding;
8. подтвердить отсутствие auto-resume;
9. только после проверки выполнить explicit cleanup evidence/staging/rollback.

Для первого positive test предпочтительна свежая копия тех же данных либо
отдельная безопасная тестовая запись. Corruption, power-loss и fault injection —
только на disposable microSD/card image.

## Safety — без изменений

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- manual writeoff требует exact
  `spool_id + source_session_id + source_run_id`.
