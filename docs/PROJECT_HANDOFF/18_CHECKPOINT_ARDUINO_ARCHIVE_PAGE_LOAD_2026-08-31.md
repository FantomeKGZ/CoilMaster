# Checkpoint 18 — Arduino archive page load recovery

Date: 2026-08-31
Branch: `arduino-ru-lcd-experiment`
Production `cmp-protocol-v1` is unchanged.

## Defect

The desktop/mobile Arduino winding archive could appear to stop loading after a page refresh. This followed the compact Session/Run display layer that was introduced to keep large exact IDs out of the primary table/card presentation.

The archive API itself was already bounded (`limit=20` from the UI, backend max page size 32) and uses opaque byte-offset cursors. The confirmed failure was in the display-only script `arduino-archive-compact-ids.js`:

- a `MutationObserver` watched the archive result subtree;
- its callback compacted the Session/Run column;
- the compaction itself mutated the same observed subtree;
- on desktop the heading was reassigned to `№` on every pass;
- this could create a self-sustaining observer/microtask feedback loop and leave the page effectively hung even though archive data loading was bounded.

## Fix

`firmware/esp32/web/shared/arduino-archive-compact-ids.js` now:

- disconnects its observer before applying its own DOM transformations;
- reconnects the observer only after the synchronous compaction is complete;
- avoids rewriting the desktop heading when it is already `№`;
- remains display-only: it performs no fetches, POSTs, archive mutation, or RUN mutation.

Desktop and mobile continue using the same shared archive controller and compact-ID layer.

## Regression coverage

`Tests/Web/check_arduino_archive_ui.js` now requires:

- `observer.disconnect()` around the compact-ID transformation;
- guarded `applyWithoutObserverFeedback()` usage;
- idempotent desktop heading mutation;
- existing exact Session/Run provenance preservation and display-only constraints.

## Safety / storage invariants

Unchanged:

- append-only autonomous winding evidence;
- exact `session_id + run_id` provenance;
- bounded archive pagination;
- no automatic physical START;
- no auto-resume;
- Arduino remains sole SSR owner;
- no automatic wire write-off;
- no DB/index migration or archive truncation.

## Commits

```text
c1b0f1af1675e67a4b0ffb6f217dc72c4bee6609  fix(web): stop Arduino archive observer feedback loop
6e67ec12ca9a1cea41bef6fc9cec7d65db08190b  test(web): guard Arduino archive observer recursion
```

CI status must be taken from exact GitHub Actions runs; do not call later documentation commits GREEN without their own exact SUCCESS.
