# Runtime storage/corruption hardening — 2026-08-12

Ветка: `cmp-protocol-v1`

## Контекст

После подтверждённого production E2E и hardware negative-test backup-during-active-winding проведён следующий repo-review: microSD unavailable / runtime-corrupted persistence / interrupted allocator transaction.

Safety boundary не менялся:

- physical START остаётся только физическим;
- ESP32/Web не управляют SSR напрямую;
- auto-resume отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и exact `spool_id + source_session_id + source_run_id`.

## Persistent ID allocator

Закрыты несколько fail-open/ambiguous окон.

### Revalidate before every allocation

Перед выдачей нового `job_id/session_id` allocator теперь заново читает authoritative:

```text
/data/winding-jobs/id-state.txt
```

и требует exact match с RAM high-water. Также:

- dangling `id-state.tmp` блокирует allocation;
- malformed/missing main state блокирует allocation;
- malformed backup блокирует allocation;
- backup high-water выше main блокирует allocation.

Ключевой commit:

```text
6a7de32bb0bffd2881e2d86cb879aca5d3ac4742
```

### Interrupted transaction at boot

`id-state.tmp` теперь не игнорируется в `begin()`. Его наличие означает незавершённую allocator transaction и allocator остаётся not-ready.

```text
a565dc5ba60c8c7e33e21eabff27587fd261c314
```

### Corrupt main + valid backup recovery

Старый recovery путь мог загрузить валидный `.bak`, затем обычной ротацией превратить повреждённый main в новый backup. Это устранено.

Теперь verified backup становится новым authoritative `id-state.txt` напрямую; повреждённый main не ротируется обратно в backup.

```text
ed4ab6e49f0bce72c0265b008baab4f556b99e96
```

### Missing main + existing backup

Отсутствующий main больше не означает автоматический first-init `0/0`, если существует `.bak`.

```text
missing id-state.txt + valid id-state.bak
→ restore backup as main
→ preserve allocator high-water
```

Только полное отсутствие main + temp + backup допускает настоящий first initialization `0/0`.

```text
424c911d745c22d40f1050ed64ef09033d940f7d
```

### Failed commit rollback

Если final temp→main rename не удался, allocator пытается восстановить предыдущий authoritative state. Candidate temp удаляется только когда rollback действительно восстановлен. Если recovery/cleanup неоднозначны, evidence остаётся и следующий boot fail-closed блокируется.

## Job runtime state store

Перед созданием нового session `JobStateStore::create()` теперь заново сканирует persisted state directory через `loadLatest()`.

Новый session разрешён только если предыдущий persisted job terminal:

```text
REJECTED
TIMED_OUT
CANCELLED
PROGRAM_COMPLETED
CLOSED_AFTER_REVIEW
```

Дополнительно новый `job_id/session_id` должен быть строго выше latest persisted identity.

Не допускается новый session поверх:

```text
CREATED
DELIVERING
WAITING_PHYSICAL_START
RUNNING
FAULT
```

Ключевые commits:

```text
97e8e75bb7e22fafdb046d5ecb5af67c733f68e5
f5f91d8abfc318c9508c12458de81a257476b2ee
```

`loadLatest()` теперь fail-closed отклоняет temporary/unexpected regular files в state directory вместо их молчаливого игнорирования. Это делает interrupted atomic state write видимым recovery fault.

Документация контракта:

```text
0214c62a1603530a53b955b8b2f46f27ac9d6264
```

## Поведение при microSD loss

Все новые guards зависят от фактического read/open persisted storage, а не только от RAM flags. Если карта пропадает между boot и новым job:

```text
allocator re-read fails
or state directory scan fails
→ new job persistence fails closed
→ UART delivery не должен считаться успешно подготовленным
```

Backup уже отдельно проверяет `/data` runtime и возвращает storage unavailable при недоступной карте.

## Verification status

Эти изменения внесены после последнего подтверждённого ESP32 build.

```text
CURRENT HEAD BUILD: NOT CONFIRMED
CI: NOT CONFIRMED
```

Перед hardware fault tests нужен новый clean PlatformIO build текущего `cmp-protocol-v1`.
