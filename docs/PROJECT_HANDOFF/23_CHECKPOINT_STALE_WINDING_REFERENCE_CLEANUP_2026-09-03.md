# Checkpoint 23 — stale winding-reference cleanup

Date: 2026-09-03

## Scope

Closed a second concrete stale user-facing Web path on active development branch `cmp-protocol-v1`.

## Finding

Current canonical winding reference is the generated SD reference bundle:

```text
/sites/reference/desktop/
/sites/reference/mobile/
```

Canonical navigation and release/build contracts already point there.

However two older variant pages still existed:

```text
firmware/esp32/web/desktop/winding-reference.html
firmware/esp32/web/mobile/winding-reference.html
```

They implemented a separate legacy reference UI and loaded:

```text
/reference/motor-reference.json
```

The current repository copy of that JSON is an empty file, so those old routes were no longer a valid user-facing reference source.

## Fix

The legacy URLs remain as compatibility aliases but now redirect to the authoritative SD reference bundle for the same UI mode.

Commits:

```text
d6b97677ac673dd9200e52bec030fc4ba2b8ac78  desktop winding-reference compatibility redirect
28dc089de996e0887d9ec0ae59ad20894146e9f3  mobile winding-reference compatibility redirect
e5002b83a1c71a8d8a8a538f1e6f3f092ce744c1  regression contract
```

Each alias retains a valid `<main>` host so the shared app shell contract remains satisfied.

## Regression protection

`Tests/Web/check_reference_search_contracts.js` now additionally requires:

- desktop legacy route redirects to `/sites/reference/desktop/`;
- mobile legacy route redirects to `/sites/reference/mobile/`;
- compatibility pages remain valid shared-shell variant HTML;
- `/reference/motor-reference.json` must not return as a data source for those user-facing compatibility routes.

The empty legacy JSON file itself was not deleted in this block because removing unrelated compatibility storage without a proven consumer audit would be unnecessary risk. Only user-facing authority was corrected.

## Exact verification

HTML code HEAD:

```text
28dc089de996e0887d9ec0ae59ad20894146e9f3
```

Verified on that exact HEAD:

```text
CMP Protocol Tests #4837
run 33729756119
completed/success

Reference Legacy Import Check #112
run 33729756146
completed/success

ESP32 Build #1876
run 33729756150
completed/success
```

Regression HEAD:

```text
e5002b83a1c71a8d8a8a538f1e6f3f092ce744c1
CMP Protocol Tests #4838
run 33729812704
completed/success
```

## Safety invariants unchanged

This block only changes static reference navigation/compatibility behavior.

No physical START, SSR, UART, repair mutation, warehouse mutation, costing or wire-writeoff behavior changed.

Still enforced:

- physical START only;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web do not directly control SSR;
- RUN_COMPLETED alone does not write off wire;
- manual wire writeoff keeps exact spool/session/run provenance.

## Next step

Continue feature-completeness only from another proven active user-facing/runtime gap. `conductor-converter`, `winding-reference`, stale Statistics and orphan Web regression coverage are now closed unless new evidence appears.
