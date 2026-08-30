# Motor Import completeness audit — 2026-08-30

Repository: `FantomeKGZ/CoilMaster`  
Working branch: `arduino-ru-lcd-experiment`  
Production/source-of-truth `cmp-protocol-v1` was not modified.

## Scope

Final feature-completeness audit of the existing Motor Import flow after the all-pages navigation checkpoint.

Reviewed current desktop/mobile UI and backend ownership for:

- JSON batch import;
- preview without mutation;
- schema/unknown-field validation;
- numeric/text ranges;
- winding-program canonical validation;
- provenance/date/URL validation;
- duplicate detection inside one import package;
- existing-motor similarity lookup;
- explicit selected import;
- desktop/mobile parity;
- bounded server responses.

## Result

Runtime/API implementation is **CLOSED / NO-CHANGE**. No functional Motor Import defect was confirmed that justified changing production behavior.

The existing semantics are intentionally operator-controlled:

- a similarity/identity match is advisory, not an automatic database-level prohibition;
- matching rows are deselected by default during preview;
- the operator may explicitly select such a row and create it;
- `/api/motors/similar` explicitly reports `creation_blocked:false`;
- therefore adding an unconditional server-side duplicate rejection would change current intended behavior and could incorrectly prevent legitimate motors that share a winding program or partial identity.

No runtime/API mutation was made.

## Confirmed existing contracts

Desktop and mobile `motor-import.html` are currently byte-identical and provide the same behavior.

Confirmed UI behavior:

- JSON array size is restricted to 1–50 records;
- unknown fields are rejected;
- required fields, string lengths and numeric ranges are checked;
- `coil_program` is canonicalized/validated;
- source classification, confidence, calculated provenance, date and HTTP(S) source URL are validated;
- duplicate-like rows inside the same package are rejected during preview;
- existing records are checked through `/api/motors/similar` before selection;
- similarity lookup failure becomes a preview error instead of silently allowing unchecked import;
- identity matches are deselected by default;
- final creation requires explicit selected rows plus operator confirmation;
- creation uses POST `/api/motors`.

Confirmed backend behavior:

- `/api/motors` repeats authoritative server-side field/range/provenance validation rather than trusting browser validation;
- `RepairRegistry::addMotor()` canonicalizes winding data and performs its append/ID journal validation;
- `/api/motors/similar` validates the winding program and scans the authoritative motors journal;
- similarity responses are bounded by `RepairRegistry::MaxListPageSize = 32`;
- response metadata exposes `returned_count`, `max_items` and `items_truncated`;
- malformed/torn motor journal evidence fails closed;
- no new cache, index or database migration was introduced.

## Regression gap closed

The confirmed gap was regression coverage, not runtime behavior.

Commit:

```text
d62d3abd39fb51af5dc48320186a0c08d780b2ec
```

`Tests/Web/check_motor_schema_ui.js` now locks:

- desktop/mobile Motor Import parity;
- batch size limit;
- allowed-field/unknown-field validation presence;
- package duplicate detection;
- fail-closed similarity preview;
- default deselection on identity match;
- explicit operator override semantics;
- explicit confirmation before selected import;
- POST `/api/motors` ownership;
- bounded similarity response and truncation evidence;
- `creation_blocked:false` advisory semantics.

This test is already executed by the mandatory `CMP Protocol Tests` workflow under the existing `Audit motor schema UI contracts` step, so no workflow change was required.

## CI status

The previous menu/navigation checkpoint is exact GREEN at:

```text
CMP Protocol Tests #4586
run 33316990592
head b26e2e0b3fd448da535c59862e0c3b59a3793c04
SUCCESS
```

The Motor Import regression commit and this documentation commit are newer than that exact GREEN head. Do **not** call them GREEN until their own current-head CMP run reports `SUCCESS`.

## Next audit target

Continue with the shared Web shell completeness audit:

1. global search;
2. recent items;
3. breadcrumbs;
4. RTC/device clock;
5. firmware/Web/SD version visibility;
6. toast/error layer;
7. then FTP/Web recovery including missing `/web`, followed by Wi-Fi profiles/static IP/network status/`coil.local`.

Safety/runtime invariants are unchanged and no new hardware test is required for this Web regression-only checkpoint.
