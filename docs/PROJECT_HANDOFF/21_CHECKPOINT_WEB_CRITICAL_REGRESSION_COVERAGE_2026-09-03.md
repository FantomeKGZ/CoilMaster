# Checkpoint 21 — Critical Web regression coverage — 2026-09-03

## Scope

Branch: `arduino-ru-lcd-experiment`

Production/source-of-truth `cmp-protocol-v1` was not changed.

This checkpoint closes the CI-coverage hardening pass that followed the Web completeness audit. Existing regression contracts that materially protect user-visible or safety-critical flows were wired into the already-running `Tests/Web/check_web_assets.js` audit instead of creating duplicate implementations.

## Exact GREEN sequence

- CMP #4791 — run `33719658051` — head `7d95d27f3b0524c6e4c431e64d93b816a09f0f50` — `completed/success`
  - autonomous winding → normal motor projection contract enabled.
- CMP #4792 — run `33719755065` — head `48ff378c6cfa330f3945411aed83a560a630058a` — `completed/success`
  - Arduino archive desktop/mobile contract enabled.
- CMP #4793 — run `33719863436` — head `58d00bb507ed15b6f2c454f736e207be2edae00a` — `completed/success`
  - calculator source-wire input contract enabled.
- CMP #4794 — run `33720105950` — head `30a71f574bb5725fd757558432d1ba09934355f3` — `completed/success`
  - linked job WORKING/STARTING role resolver contract enabled.
- CMP #4795 — run `33720233161` — head `135b7042906f29c98d6de8c77d5d0e85d1620061` — `completed/success`
  - exact RUN_WIRE issue transaction contract enabled.
- CMP #4796 — run `33720351580` — head `7c138b92c29d5d33b2c1e51b4449686a3d337b05` — `completed/success`
  - spool↔material bridge Web and append-only store contracts enabled.

## What is now protected by the normal CMP Web audit

The existing Web audit now directly invokes regression contracts for:

- client creation required/optional field behavior;
- FTP/Web recovery;
- remote backup desktop/mobile parity and restore staging;
- repair material card/writeoff/correction/RUN_WIRE UI parity;
- CRUD page separation and single-flight create flows;
- dashboard Arduino job history;
- client CRM/cash flow;
- autonomous winding motor projection;
- Arduino archive UI;
- calculator source-wire input;
- linked winding job role resolution;
- exact RUN_WIRE transaction recovery/accounting;
- spool↔material bridge Web confirmation and append-only store integrity.

## Safety invariants preserved

- no automatic physical START;
- no auto-resume after reboot;
- Arduino remains the sole SSR owner;
- ESP32/Web does not directly control SSR;
- `RUN_COMPLETED` alone does not deduct wire;
- RUN_WIRE remains explicit/manual and tied to exact `spool_id`, `source_session_id`, and `source_run_id`;
- append-only transaction/evidence logs remain fail-closed;
- no silent STARTING→WORKING fallback;
- restore/apply remains explicit and operator-controlled.

## Current state at checkpoint creation

Last exact GREEN implementation head before this documentation commit:

`7c138b92c29d5d33b2c1e51b4449686a3d337b05`

CMP #4796 / run `33720351580` is exact `completed/success` for that head.

The documentation commit itself must receive its own exact CI result before it may be called GREEN.

## Next work

Stop adding regression files merely because they are currently orphaned. The next pass is a real user-facing completeness audit of the current desktop/mobile site:

1. inspect settings and service pages for visible controls that do not complete their advertised action;
2. inspect repair/client/motor detail pages for incomplete navigation or dead actions;
3. inspect warehouse/material/report/history pages for placeholder/empty states that are actually unfinished rather than legitimate empty-data states;
4. inspect calculator/converter pages for user-visible inconsistencies;
5. fix only a confirmed functional gap; otherwise record the audited block as NO-CHANGE.

All modifications continue only on `arduino-ru-lcd-experiment`; production remains untouched unless the user gives a separate direct instruction.
