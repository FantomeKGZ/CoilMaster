# Checkpoint 21 — Web regression reachability audit

Date: 2026-09-03

## Scope

Closed the remaining PROJECT_HANDOFF item to audit orphaned Web regression contracts and rechecked the historical stale Statistics-page concern on the current development branch.

Current branch policy remains authoritative from checkpoint 20:

- `cmp-protocol-v1` — only active development/source branch;
- `main` — stable/ready state only;
- retired `arduino-ru-lcd-experiment` is not a development source.

## Cash UI regression restored to the CI graph

`Tests/Web/check_cash_ui.js` existed and still protected active production Cash UI behavior, but it was not reachable from either the CMP workflow or the `check_web_assets.js` require graph.

Commit:

```text
1c2bc1917bd3c78dac45a431fb623740e7e5696d
test(web): wire cash UI contract
```

It is now required from `Tests/Web/check_web_assets.js` and therefore runs with the normal Web asset audit.

The contract protects, among other things:

- bounded repair/payment paging;
- exact uint64 minor-unit money rendering;
- append-only PAYMENT/CORRECTION writes;
- explicit confirmation;
- exact repair identity;
- dedicated Cash navigation;
- no payment delete/replace/patch;
- no job, hardware, START or warehouse mutation shortcuts.

## Permanent orphan-contract guard

Commit:

```text
7c869371993a87562777863fef7e80161c3dd77f
test(web): guard orphan regression contracts
```

`Tests/Web/check_ci_trigger_contracts.js` now builds a reachability graph for every `Tests/Web/check_*.js` file starting from Web tests directly executed by `cmp-protocol-tests.yml` and following local `require('./check_*.js')` edges.

The audit fails if:

- a workflow references a missing Web regression file; or
- any `check_*.js` exists but is not reachable from the CI graph.

The first run intentionally exposed one pre-existing orphan:

```text
CMP Protocol Tests #4824
run 33728459826
head 7c869371993a87562777863fef7e80161c3dd77f
failure only in: Audit CI trigger coverage
orphan: check_ru_hall_calibration_experiment.js
```

All other already-run CMP checks in that workflow continued to pass; this was an audit discovery, not a production behavior regression.

## RU Hall regression retained and made reachable

Inspection showed `check_ru_hall_calibration_experiment.js` is not dead branch-only baggage. It still protects current Hall local-control and safety behavior, including:

- 15-second calibration run and timeout guards;
- physical local start through keypad `A` or physical START;
- SSR fail-safe ownership/interlock on Arduino;
- bounded RU LCD calibration states;
- no ESP32/Web Hall motor-start endpoint;
- no Web SSR control;
- current no-extra-`#` start-confirm wording while EEPROM apply remains separately confirmed.

Therefore the test was retained rather than deleted.

Commit:

```text
29f4749ba917446b88ea625fc31e811baa849a93
test(web): keep RU Hall contract reachable
```

It is now reachable and executed through the CI trigger-contract graph.

Exact verification:

```text
CMP Protocol Tests #4825
run 33728574447
head 29f4749ba917446b88ea625fc31e811baa849a93
completed/success
```

The new orphan-reachability audit and the existing Hall/Web audits all completed successfully.

## Stale Statistics page recheck

The historical empty/placeholder Statistics page remains removed on current `cmp-protocol-v1`:

```text
firmware/esp32/web/desktop/statistics.html -> absent
```

Previous handoff documentation already recorded removal of both desktop/mobile Statistics placeholders. No production page restoration or deletion was needed in this block.

## Safety invariants unchanged

No production firmware or business mutation behavior was changed by this block.

Still enforced:

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- RUN_COMPLETED alone does not write off wire;
- wire write-off remains manual and tied to exact spool/run/session provenance.

## Current checkpoint

Current verified code/test HEAD before this documentation-only commit:

```text
29f4749ba917446b88ea625fc31e811baa849a93
CMP #4825 completed/success
```

Uno resource checkpoint from checkpoint 20 remains:

```text
Flash: 31114 / 32256; 1142 bytes free
RAM:   1227 / 2048; 821 bytes free
```

## Next step

The orphaned-Web-regression and stale Statistics-page items are closed.

Next work should be chosen from an actually unresolved functional/runtime item on current `cmp-protocol-v1`, preserving the regression graph added here. Do not resume speculative Uno compiler/parser micro-optimization unless a measured resource need appears.
