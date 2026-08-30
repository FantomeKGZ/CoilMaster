# Mobile linked winding role parity — 2026-08-30

Branch: `arduino-ru-lcd-experiment`

## Finding

Desktop linked winding already used the canonical motor winding-version contract, but mobile `winding-job.html` was still on the older motor-level flow:

- it read `motor.coil_program` directly;
- `repeat_target` remained operator-editable;
- it did not query `/api/motors/winding/latest`;
- STARTING had no independent authoritative program/repeat handling.

The ESP32 backend remained authoritative and compared exact role/program/repeat before persistence/UART, so this stale mobile UI did not bypass machine safety. It could, however, submit a valid-looking mobile request that the server rejected with role/program/repeat mismatch, especially for STARTING or manually changed repeat counts.

## Runtime/UI fix

Commit:

```text
9fd17d1adde4d6b4eb92b3c7067b742fe4a1610d
```

`firmware/esp32/web/mobile/winding-job.html` now matches the existing desktop contract:

- canonical role selector `WORKING` / `STARTING`;
- `/api/motors/winding/latest?motor_id=...` is the versioned authoritative source;
- `working_program + working_repeat_target` are used for WORKING;
- STARTING uses `starting_program + starting_repeat_target` only when `starting_present=true`;
- missing STARTING disables the option and a requested `role=starting` fails closed;
- legacy motor records are accepted only as WORKING fallback;
- no STARTING → WORKING substitution is allowed;
- program and `repeat_target` are read-only in linked winding UI;
- exact spool selection remains mandatory;
- physical START remains mandatory for every repeat;
- `RUN_COMPLETED` still never performs automatic wire write-off.

## Regression protection

`Tests/Web/check_linked_job_winding_role.js` was extended so the already mandatory CMP step now protects both desktop and mobile UI contracts in addition to backend role resolution.

The first test-only commit `5ff1cd7208bfe32c971f02877742d71c8d3cec54` contained an assertion-string syntax error and is superseded. It must not be used as GREEN evidence.

Final regression commit:

```text
063c974d3695e3998a4f30a6e437b305d5233835
```

The test now checks both variants for:

- canonical role selector;
- read-only repeat target;
- authoritative winding-version endpoint;
- independent role state;
- STARTING presence gate;
- disabled missing STARTING;
- fail-closed requested STARTING;
- explicit prohibition of STARTING→WORKING fallback;
- operator copy stating that role/program/repeat are authoritative and locked.

## Exact CI evidence

Runtime/UI commit `9fd17d1...`:

```text
ESP32 Build #1793
run 33319091625
completed / success

Arduino RU LCD Build #222
run 33319091602
completed / success
```

Final regression/current test HEAD `063c974d...`:

```text
CMP Protocol Tests #4603
run 33319146379
completed / success
```

This is sufficient evidence that the mobile Web change compiles in the ESP32 project, does not disturb the Arduino RU LCD build, and the linked winding role contract is enforced in the mandatory CMP suite.

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- linked job requires exact active spool identity;
- backend revalidates exact motor winding role/program/repeat before persistence/UART;
- wire write-off remains manual and exact-run-bound.

## Next

Continue from downstream entry points into `winding-job.html`: verify that repair/motor UI actions route intentionally to WORKING or STARTING and do not reintroduce a role-blind user path. Use current branch files directly rather than stale code-search results.
