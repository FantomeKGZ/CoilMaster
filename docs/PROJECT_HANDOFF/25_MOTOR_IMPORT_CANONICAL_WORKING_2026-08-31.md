# Motor import canonical WORKING version — 2026-08-31

Branch: `arduino-ru-lcd-experiment`.
Production `cmp-protocol-v1` was not modified.

## Proven gap

Desktop/mobile `motor-import.html` previously created only the legacy motor registry row through `POST /api/motors`. The imported `coil_program` therefore appeared only through the legacy fallback and did not create an append-only canonical `MotorWindingVersionStore` version.

The import schema also rejected `repeat_target` as an unknown field even though `/api/motors` already supports it.

## Fix

The import flow now preserves the existing persistence/recovery boundaries instead of introducing a new hidden two-journal transaction:

1. create the motor card through the existing `POST /api/motors` endpoint;
2. immediately create the first canonical winding version through the existing authoritative `POST /api/motors/winding/role` endpoint;
3. use `expected_winding_version_id=0`, `role=WORKING`, the imported canonical program, repeat target, coil pitch and available conductor metadata;
4. tag the version comment as `MOTOR_IMPORT`.

No backend C++ mutation/recovery semantics were changed.

If the motor card is created but the canonical WORKING append fails, the import is explicitly marked `ЧАСТИЧНО`; the created row is disabled in the current import session so an automatic duplicate motor is not created by retry. The operator is directed to the normal motor edit flow to complete the canonical winding record.

Desktop and mobile import pages remain byte-identical.

## Commits

```text
946b1c7d5780150aecff5e6c40bca805182bab20  desktop implementation
83a397a6aee72bd2b4c0e1fc35f5d6cfe01be0b4  mobile parity
4cef431645882ee440f14d476094fe900c9ed098  regression contract
```

## Verification

Intermediate desktop-only commit:

```text
CMP #4727 / run 33367564962 / FAILURE
```

This was the expected existing desktop/mobile byte-parity assertion before the mobile page was updated; it was not a firmware/runtime failure.

Final implementation parity:

```text
83a397a6aee72bd2b4c0e1fc35f5d6cfe01be0b4
CMP #4728 / run 33367617831 / SUCCESS
ESP32 #1843 / run 33367617835 / build job SUCCESS
Arduino RU LCD #274 / run 33367617854 / compare-builds job SUCCESS
```

Desktop implementation also independently compiled:

```text
946b1c7d5780150aecff5e6c40bca805182bab20
ESP32 #1842 / run 33367564927 / build job SUCCESS
Arduino RU LCD #273 / run 33367564929 / compare-builds job SUCCESS
```

Final regression head:

```text
4cef431645882ee440f14d476094fe900c9ed098
CMP #4729 / run 33367655871 / SUCCESS
```

## Safety

Unchanged:
- no automatic physical START;
- no Web/ESP32 SSR control;
- no auto-resume;
- no automatic RUN_WIRE writeoff;
- linked wire writeoff still requires exact `spool_id + source_session_id + source_run_id`;
- append-only winding versions remain authoritative;
- no new recovery/WAL boundary was added.

## Next

Continue the feature-completeness audit with the shared Web shell/navigation/global search/recent/breadcrumbs/time/version/toasts block. Only proven incomplete items should be changed.
