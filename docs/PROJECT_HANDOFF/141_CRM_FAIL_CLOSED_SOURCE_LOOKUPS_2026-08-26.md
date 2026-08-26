# Checkpoint 141 — CRM fail-closed source lookups (2026-08-26)

## Status

Implementation complete. Production source is compile-GREEN; final contract revalidation is pending because the first CMP run for the final test commit ended with GitHub Actions `startup_failure` before any job was created.

## Source changes

RepairRegistry existence checks no longer collapse catalog read/integrity failure into ordinary not-found:

- `clientExists(uint32_t, bool& found)`
- `motorExists(uint32_t, bool& found)`

The old one-argument convenience forms are removed.

Updated production callers use an explicit success channel plus `found`:

- RepairRegistry add/validation path
- RepairRegistry lookup Web
- Autonomous winding Web
- Cash payment/balance Web
- Repair intake coordinator

HTTP/read semantics now distinguish:

- valid catalog + missing entity -> ordinary `not_found`
- malformed/unreadable/integrity-failed catalog -> fail closed / integrity error

No repair intake pending/recovery ordering was changed.

## Key commits

- Registry/API and caller migration through final source: `35ba207a678547189a550aea9257ed1660d9853a`
- Final CRM contract: `18a0297bd9260d1f41db5e9460813695d174542d`

## Confirmed CI

- ESP32 Build `#1612`, run `32983761012`: SUCCESS on source commit `35ba207a...`
- CMP Protocol Tests `#3656`, run `32983760884`: SUCCESS on source commit `35ba207a...`; all 68 host audit steps succeeded
- CMP Protocol Tests `#3657`, run `32983999245`: `startup_failure` on final contract commit `18a0297b...`; GitHub created zero jobs, so this is not a code/test assertion failure

The next normal CMP-triggering commit must be used to revalidate the final contract before checkpoint 141 is promoted to the canonical GREEN read-order in `00_READ_FIRST.md`, `01_CURRENT_STATE.md`, and `06_ACTIVE_WORK_AND_NEXT_STEPS.md`.

## Safety invariants

Unchanged:

- no automatic physical START
- no automatic START between repeats
- no auto-resume after reboot
- Arduino remains the SSR owner
- ESP32/Web do not directly drive SSR
- RUN_COMPLETED alone never deducts wire
- material write-off remains explicit and exact-provenance based
- restore remains explicit/operator-only/transactional/fail-closed
