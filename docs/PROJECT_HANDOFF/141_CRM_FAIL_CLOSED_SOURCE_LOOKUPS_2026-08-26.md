# Checkpoint 141 — CRM fail-closed source lookups (2026-08-26)

## Status

GREEN. Production source and final contract are validated by later successful CMP/ESP32 runs after the transient GitHub Actions startup/queue failure cleared.

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

Original direct evidence:

- ESP32 Build `#1612`, run `32983761012`: SUCCESS on source commit `35ba207a...`
- CMP Protocol Tests `#3656`, run `32983760884`: SUCCESS on source commit `35ba207a...`; all mandatory host audits succeeded
- CMP Protocol Tests `#3657`, run `32983999245`: infrastructure `startup_failure` before any job was created; not a code/test failure

Later revalidation containing the final 141 source + contract:

- CMP Protocol Tests `#3663`, run `33034665166`: SUCCESS
- ESP32 Build `#1616`, run `33034665123`: build job SUCCESS
- CMP Protocol Tests `#3664`, run `33034707952`: SUCCESS on later contract-bearing commit

Checkpoint 141 is therefore part of the canonical GREEN foundation.

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
