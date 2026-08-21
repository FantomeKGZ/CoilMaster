# UART-протокол и рабочий цикл намотки

Дата актуализации: **2026-08-21**  
Ветка: `cmp-protocol-v1`

Этот документ описывает текущую production CMP1 semantics. Старые binary CMP документы в `Docs/Protocol/` и `Shared/Protocol/` являются legacy/host-test context.

## Production owners

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/arduino/src/main.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
firmware/esp32/src/main.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
```

Arduino side uses SoftwareSerial on project pins A1/A2; ESP32 peer uses its UART side. Production CRC is CRC-16/MODBUS from `CM_Cmp1Crc.h`.

## ESP32 -> Arduino JOB

Current negotiated job frame is:

```text
CMP1|JOB|JOB_ID|SESSION_ID|TYPE|COIL_COUNT|TURNS|R<REPEAT_TARGET>|C|CRC16
```

Where:

- `JOB_ID` and `SESSION_ID` are non-zero;
- `TYPE` is `WORKING` or `STARTING`;
- `COIL_COUNT` is bounded by the production job structure;
- `TURNS` is the comma-separated program;
- `REPEAT_TARGET` is non-zero;
- `C` requests CRC-protected job replies;
- CRC is calculated over the payload before `|CRC16`.

A remote JOB only prepares/loads the Arduino program. It never means physical START.

## JOB_ACK

Negotiated reply:

```text
CMP1|JOB_ACK|JOB_ID|ACCEPTED|DETAIL|C|CRC16
CMP1|JOB_ACK|JOB_ID|REJECTED|DETAIL|C|CRC16
```

Legacy reply without `C|CRC16` remains accepted only for staged compatibility where allowed by the current parser. A negotiated/truncated/bad-CRC reply must not silently downgrade to legacy.

ESP32 job delivery retries are bounded. Timeout/rejection/acceptance are delivery state only and never authorize SSR.

## JOB cancellation / ghost-job recovery

Current ESP32 cancellation behavior:

- if a queued JOB was never transmitted, local cancellation can complete without remote handshake;
- once a JOB may have reached Arduino, cancellation switches to idempotent `JOB_CANCEL` handshake;
- this prevents a lost `JOB_ACK` from leaving a ghost job on Arduino.

Cancel frame:

```text
CMP1|JOB_CANCEL|JOB_ID|CRC16
```

Arduino reply:

```text
CMP1|JOB_CANCEL_ACK|JOB_ID|CANCELLED|DETAIL|C|CRC16
CMP1|JOB_CANCEL_ACK|JOB_ID|REJECTED|DETAIL|C|CRC16
```

Important implemented semantics:

- already-clear Arduino state returns successful `ALREADY_CLEAR` rather than making absence a permanent error;
- no-run remote job can be cancelled safely;
- active physical-run evidence prevents unsafe cancellation;
- physical fallback `D -> * -> # -> D` sends:

```text
CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C|CRC16
```

only after Arduino proves no remote job is physically active;
- ESP32 correlates `job_id=0 + ALL_CLEAR` to the current/persisted job and follows the normal audited cancellation path;
- `ALL_CLEAR` never means `RUN_COMPLETED` and never creates wire/material writeoff.

This recovery block is implemented/closed. Reopen only for a concrete regression.

## Arduino -> ESP32 winding events

Linked remote run:

```text
CMP1|EVT|RUN_STARTED|SESSION_ID|RUN_ID|0|CRC16
CMP1|EVT|RUN_COMPLETED|SESSION_ID|RUN_ID|COMPLETED_RUNS|CRC16
```

Standalone/local Arduino run includes immutable local program snapshot:

```text
CMP1|LOCAL_EVT|TYPE|SESSION_ID|RUN_ID|COMPLETED_RUNS|WORKING_OR_STARTING|COIL_COUNT|TURNS|CRC16
```

Strict parser rules include non-zero session/run IDs, bounded numeric fields, no extra fields, valid CRC, `RUN_STARTED -> completed_runs == 0` and `RUN_COMPLETED -> completed_runs > 0`.

## ESP32 ACK/NACK for run events

Current replies are CRC-protected:

```text
CMP1|ACK|RUN_ID|DETAIL|CRC16
CMP1|NACK|RUN_ID|DETAIL|CRC16
```

Arduino only removes/retries a queued run event according to valid reply semantics. Lost or malformed replies do not convert into successful completion of transport persistence.

## Repeat semantics

Example:

```text
program = [38, 38]
repeat_target = 6
```

This is one JOB with six physical RUNs. Each full program pass gets its own `run_id` and requires a new physical START.

```text
RUN 1 -> physical START -> RUN_STARTED -> RUN_COMPLETED
RUN 2 -> physical START -> RUN_STARTED -> RUN_COMPLETED
...
```

No automatic START exists between repeats.

## Winding journal

Authoritative linked event journal:

```text
/data/winding-runs/events.ndjson
```

Full validation uses:

```text
WindingJournalQuery::validateAll()
WindingJournalTransitionAudit::validate()
```

Do not reintroduce cursor pagination as authoritative full-file validation.

The journal and transition audit reject invalid ordering/identity such as completion without start, mismatched session/run, invalid completed counters or conflicting active transitions.

## Physical production flow

```text
ESP32 prepares persisted linked JOB
-> UART JOB delivery
-> Arduino validates and ACKs
-> operator presses physical START
-> Arduino owns SSR/Hall winding
-> RUN_STARTED / RUN_COMPLETED
-> ESP32 persists exact run evidence
-> operator performs explicit manual material writeoff
```

Remote acceptance, cancel ACK and event ACK never directly control SSR.

## Reboot safety

Neither board may interpret reboot recovery as permission to start/continue physical movement. Recovery is state reconciliation/manual review only.

Reboot/cancel/ALL_CLEAR must never synthesize:

```text
physical START
RUN_COMPLETED
material writeoff
```

## Current verification reference

Production commit `e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00` has:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Current Arduino Uno Build and current-head two-board hardware smoke remain separate verification gates as documented in `00_READ_FIRST.md` / checkpoint `61`.
