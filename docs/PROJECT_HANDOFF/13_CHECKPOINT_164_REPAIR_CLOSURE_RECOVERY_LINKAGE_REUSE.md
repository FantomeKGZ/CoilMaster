# Checkpoint 164 — repair closure recovery linkage reuse

Дата: **2026-08-29**  
Ветка: **`arduino-ru-lcd-experiment`**  
Production `cmp-protocol-v1` не изменён.

## Статус

**GREEN**.

## Что изменено

`RepairClosureGuard::canClose()` раньше выполнял:

1. `JobRecovery::evaluate()`;
2. внутри recovery — authoritative latest-state scan + immutable snapshot identity validation;
3. затем `JobDisplayRecovery::load()` повторно открывал тот же snapshot только для проверки repair linkage.

Теперь `JobRecovery::evaluate()` загружает immutable `JobSnapshot` один раз и сохраняет уже проверенный `JobLinkage` в `JobRecoveryInfo`. `RepairClosureGuard` использует этот linkage напрямую и не повторяет snapshot read.

`JobSnapshotStore::load()/parse()` остаётся fail-closed и по-прежнему проверяет:

- exact nonzero job/session identity;
- linkage pair validity;
- canonical program type;
- nonzero bounded repeat target;
- nonzero bounded coil count;
- complete turns array.

Recovery disposition, `mayCreateNewJob`, manual-review rules, timeout/delivering ambiguity and all physical START/SSR semantics не изменены.

## Коммиты

```text
5c78c0881d1d1ed8c136e73ddfb63de6878d579a  retain validated linkage in recovery info
12bf02f61fa49f16ad39fbb61734441807cbcb07  load snapshot once and retain linkage
009eec44f6bf1783052436758afa5c49c94766ea  repair closure reuses validated linkage
```

## Проверенный CI

Runtime `009eec44f6bf1783052436758afa5c49c94766ea`:

```text
CMP Protocol Tests #4003  run 33264518095 / SUCCESS
ESP32 Build #1772         run 33264518093 / SUCCESS
Arduino RU LCD #196       run 33264518066 / SUCCESS
```

## Adjacent audit / NO-CHANGE

`JobStateStore::writeAtomic()` post-commit target reread остаётся **NO-CHANGE**: это verification/rollback integrity boundary, а не лишний growing-journal pass.

Local autonomous `completedTaskExists()` mutation-time reread также остаётся **NO-CHANGE** как authoritative TOCTOU proof перед assignment append.

## Следующий кандидат

`main.cpp::restoreLatestJobState()` после `JobRecovery::evaluate()` всё ещё повторно открывает тот же immutable snapshot через `JobDisplayRecovery::load()` для восстановления полного display/program state. Проверять его отдельно: оптимизировать только если validated snapshot можно безопасно передать без ослабления recovery contracts и без нежелательного RAM/flash роста.
