# Продолжение CoilMaster

Рабочая/source-of-truth ветка:

```text
cmp-protocol-v1
```

`main` не использовать как источник реализации.

Перед продолжением читать в таком порядке:

```text
AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```

Старые numbered checkpoints в `docs/PROJECT_HANDOFF/` — история выполнения и evidence старых baselines. Их `next`/`pending` разделы не являются текущей очередью работ.

Текущий подтверждённый repo baseline:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

JOB cancel/recovery уже реализован и не является активной задачей без конкретной регрессии.

Текущая проверочная очередь: Arduino Uno Build current HEAD, затем targeted two-board UART/hardware smoke при доступном стенде, после чего — только concrete failures или текущая явно запрошенная функциональность.
