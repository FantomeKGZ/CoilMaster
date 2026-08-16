# CoilMaster — продолжение проекта

Дата обновления: **2026-08-16**

Репозиторий: `FantomeKGZ/CoilMaster`  
Единственная source-of-truth ветка: `cmp-protocol-v1`

## Начать отсюда

1. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md` — самый свежий
   checkpoint: backend fail-closed lock на persisted apply evidence, explicit
   cleanup exception и scheduler wait-state.
2. `29_TRANSACTIONAL_APPLY_POST_REBOOT_UI_GUARD_2026-08-16.md` — post-reboot
   UI guard для `STALE` recovery evidence.
3. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` — активная работа и точное продолжение,
   включая завершённый общий CRC рабочего CMP1.
4. `01_CURRENT_STATE.md` — текущее состояние и оценка готовности.
5. `02_ARCHITECTURE_AND_HARDWARE.md` — аппаратная архитектура и питание.
6. `03_PROTOCOL_AND_WINDING_FLOW.md` — фактический UART/CMP1 flow.
7. `09_KEY_FILES_INDEX.md` — индекс production-файлов.
8. `08_WORK_RULES_AND_VERIFICATION.md` — правила изменения и проверки.

Файлы `12`–`29` сохраняют предыдущие checkpoints и не заменяют текущий код.
`11_FULL_BRANCH_AUDIT.md` — историческая карта, не source of truth.

## Источник истины

Приоритет:

1. текущий код `cmp-protocol-v1`;
2. фактический результат актуального build/Actions;
3. `30_TRANSACTIONAL_APPLY_BACKEND_STALE_LOCK_2026-08-16.md`;
4. `06_ACTIVE_WORK_AND_NEXT_STEPS.md` и `01_CURRENT_STATE.md`;
5. остальные handoff и тематические документы.

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
- восстановление backup не выполняется автоматически и не продолжается после
  reboot;
- fail-closed semantics не ослаблять ради UI convenience.

## Текущая точка

CoilMaster v1 оценивается в **93%**. Stable external-power Wi-Fi и read-only
apply preflight до `READY → reboot → STALE → cleanup` подтверждены пользователем
на реальном устройстве. Operator-only transactional restore apply уже реализован
с double confirmation, intent journal, CRC32-проверенными `.part` и automatic
rollback при обычной runtime-ошибке.

Post-reboot recovery теперь закрыт в двух слоях: UI блокирует новые backup/restore
действия при `STALE`, а backend также считает surviving `APPLY_JOURNAL.tsv` /
`APPLY_RESULT.txt` busy evidence до explicit cleanup. Scheduled backup при этом
переходит в `WAITING_RESTORE_CLEANUP` и не расходует дневную попытку. Firmware
ESP32 build и protocol/web regression audit успешны.

Следующий обязательный hardware-gate — безопасный **positive transactional apply
test** с последующим reboot, проверкой `STALE`, ручной проверкой данных и explicit
cleanup. Fault-injection разрешён только на disposable card/image.
