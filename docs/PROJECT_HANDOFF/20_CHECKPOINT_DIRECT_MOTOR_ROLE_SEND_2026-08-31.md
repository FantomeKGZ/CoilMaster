# Checkpoint — direct motor WORKING / STARTING send from motor card

Date: 2026-08-31
Branch: `arduino-ru-lcd-experiment`
Production `cmp-protocol-v1` remains untouched.

## User goal

Allow a saved motor winding to be sent to the station directly from the motor card even when the operator does not create/register a repair (for example, winding the operator's own motor).

## Implemented contract

Desktop motor card keeps the role buttons under WORKING and STARTING, but the shared helper no longer searches for an OPEN repair and no longer navigates to `winding-job.html`.

On click it now:

1. rereads `/api/motors/winding/latest?motor_id=...`;
2. uses the exact current WORKING or STARTING program + repeat target (WORKING may fall back to legacy motor fields when no versioned record exists; STARTING remains fail-closed without an authoritative versioned STARTING role);
3. checks `/api/status` and requires `job_creation_ready === true`;
4. POSTs only `type + turns + repeat_target` to `/api/jobs`;
5. deliberately omits `repair_id`, `motor_id`, and `spool_id`, therefore the backend creates an unlinked service job;
6. verifies the response says `linked:false`, `repair_id:null`, `motor_id:null`, `spool_id:null`, `spool_selection_saved:false`, and `automatic_wire_writeoff_allowed:false`.

The existing repair-specific actions remain available in the repair list and still open the linked `winding-job.html?repair_id=...&role=...` flow when production repair provenance and exact spool selection are desired.

## Safety invariants preserved

- no Web/ESP32 automatic physical START;
- Arduino remains sole SSR owner;
- direct motor-card action only queues a remote job; actual winding still begins with physical START;
- no repair is created implicitly;
- no spool is selected for this unlinked service job;
- no automatic wire writeoff is enabled;
- no RUN evidence is synthesized;
- linked repair flow and its exact spool/source provenance remain unchanged.

## Commits

- `258eaa3d1ca9fd1f84196baa8d420adea3bc062f` — direct service send helper implementation.
- `8495130cf916c47cda2b15415764fba09945214b` — regression contract updated for direct unlinked send.

The intermediate implementation commit triggered CMP #4661 and failed because the previous regression intentionally required repair navigation and forbade direct POST. The regression was then updated to the new user-selected contract. Do not call the new test commit GREEN until its own exact CI run succeeds.
