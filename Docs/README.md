# CoilMaster legacy `Docs/` tree

This capitalized `Docs/` directory contains early architecture/protocol/development documents from the foundation phase.

It is **not** the current project-status or active-work source.

For current work use:

```text
/AGENTS.md
/docs/PROJECT_HANDOFF/00_READ_FIRST.md
/docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
/docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
/docs/AI_AGENT/
```

Important distinction:

```text
Docs/CMP_Protocol_v1.md and Docs/Protocol/*
```

may describe early protocol work. Production ESP32<->Arduino communication currently uses text `CMP1|...` implemented by:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
Shared/CMP1Text/CM_Cmp1Crc.h
```

`Shared/Protocol/` / older binary CMP material is host-test/legacy context, not a drop-in production wire implementation.

Do not resume a task from this legacy tree merely because it is labelled unfinished or foundation development.
