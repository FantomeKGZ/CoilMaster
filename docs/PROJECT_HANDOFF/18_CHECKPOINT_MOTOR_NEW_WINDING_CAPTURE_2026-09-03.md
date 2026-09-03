# Checkpoint 18 — Motor creation captures canonical WORKING / STARTING winding

Date: 2026-09-03
Branch: `arduino-ru-lcd-experiment`
Production/source-of-truth branch `cmp-protocol-v1` was not modified.

## Confirmed GREEN base for this checkpoint

The implementation series through:

`bb6af586d51ace86a584ec3f24e5a96b7d4e9e0d`

was confirmed by exact GitHub Actions result:

- `CMP Protocol Tests #4770`
- run: `33708384422`
- head: `bb6af586d51ace86a584ec3f24e5a96b7d4e9e0d`
- result: `completed/success`

Do not treat later documentation-only commits as GREEN unless their exact run is also confirmed.

## Closed block

New motor creation now captures the first canonical versioned winding instead of leaving all winding details for a later edit.

### WORKING

`WORKING` is mandatory when creating a new motor.

The desktop and mobile creation forms now collect:

- winding program;
- repeat target;
- wire material: `CU` / Медь or `AL` / Алюминий;
- one through five physical wire diameters.

The legacy motor-card `coil_program` and `repeat_target` are derived from the WORKING input for compatibility and similarity search.

After `/api/motors` creates the motor identity, the UI appends the first versioned WORKING record through the existing append-only endpoint `/api/motors/winding/role` with `expected_winding_version_id=0`.

### STARTING

The form has an explicit `Есть пусковая обмотка` toggle.

When disabled:

- no STARTING record is submitted;
- the motor receives only its WORKING winding version.

When enabled:

- STARTING has its own program, repeat target, material and wire list;
- STARTING is appended only after WORKING succeeds;
- the returned WORKING version id is used as `expected_winding_version_id` for STARTING.

This preserves the existing invariant that STARTING cannot be the first/orphan winding version.

## Wire input and canonical storage

User-facing wire input accepts:

- `;` or `:` as list separators, including mixed use;
- comma or dot as the decimal separator;
- spaces, which are ignored;
- duplicate diameters;
- up to five physical wires for each role independently.

Examples:

- `0.60;0,70`
- `0.60:0,70;0.80`
- `0.60;0.60:0.70;0.75:0.80`

Input is normalized into the existing canonical `conductors` format. Diameter is stored in hundredths of a millimetre and equal diameters are aggregated using `xN`, for example:

`0.60;0.60;0.70` with copper becomes `CU:60x2+CU:70x1`.

No parallel wire storage field was introduced.

## Backend conductor capacity

`MotorWindingRoleSpec::MaxConductors` was raised from 4 to 5.

The parser, validation and serialization use this shared model constant, so five distinct conductor components are supported without introducing a separate parser-side limit.

## Motor edit UI

Desktop and mobile `motor-edit.html` now expose, independently for WORKING and STARTING:

- material selector;
- friendly wire diameter list;
- existing append-only version save;
- optimistic `expected_winding_version_id` conflict protection.

Simple canonical conductor data is parsed back into the friendly fields.

A technical raw canonical fallback is retained for old, mixed-material or otherwise non-simple records which cannot be safely represented as one material plus at most five physical wires. This prevents legacy data loss.

## Motor details UI

Desktop and mobile `motor-details.html` now render canonical conductors as human-readable material and diameter information, for example:

`CU:95x2+AL:100x1` → `Медь: 0,95 мм × 2; Алюминий: 1,00 мм`.

Unknown/legacy canonical values fall back to their raw representation.

The same formatter is used for current winding data, version history and repair before/after comparison.

## Tests / CI coverage

`Tests/Web/check_motor_edit_ui.js` was extended to cover:

- WORKING and optional STARTING creation;
- CU / AL material selection;
- one-to-five wire capture;
- `:` / `;` separators;
- comma decimal normalization;
- duplicate aggregation into canonical `xN`;
- append-only role endpoint usage and version ordering;
- editor friendly fields and raw fallback;
- details formatter;
- absence of Web SSR control.

The test existed previously but was not invoked by `CMP Protocol Tests`. The workflow now runs it explicitly.

`Tests/Web/check_motor_winding_version_schema.js` was updated from the obsolete four-component expectation to the new five-component model contract.

## Implementation commits

- `64778406f919a32881bb30ad53e09109e005b257` — allow five conductor components
- `135075a3d07f22adfd43651e1818762aef8167ad` — desktop new-motor canonical winding capture
- `22851750e5adb1e43ba9a0c63cc5fa092e789a19` — mobile new-motor canonical winding capture
- `d6b1c2a6100013e32bfb6d1845bed029c927fffa` — desktop friendly winding edit
- `07dbf8b4a5eee79bd40a2db19da7dbf8ba29ccf3` — mobile friendly winding edit
- `7e44414a95dbfb78df7275c57d6fe9ab6669ed1f` — desktop friendly winding details
- `57f48c3197d5cf537de124794bd853a4eaa40199` — mobile friendly winding details
- `21dfde205d2f2d325fc379238535938cdd5a29d7` — regression coverage
- `1039c203412f8ecf47f5dd138215638d217e2682` — run winding UI contract in CMP workflow
- `bb6af586d51ace86a584ec3f24e5a96b7d4e9e0d` — update schema contract to five conductors

## Atomicity note

Motor identity and winding history remain separate append-only stores/endpoints. The creation UI therefore performs motor creation followed by versioned winding append. It does not fabricate rollback semantics that the storage architecture does not provide. If the motor card succeeds but a winding append fails, the UI reports that partial state explicitly instead of silently treating it as success.

## Safety invariants unchanged

- no automatic physical START;
- no reboot auto-resume;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` does not write off wire;
- wire writeoff remains manual and bound to exact spool/session/run provenance.

## Next step

Before starting another feature block, verify the exact CI result for the documentation HEAD created by this checkpoint. Then use this file together with `00_READ_FIRST.md`, `01_CURRENT_STATE.md` and `06_ACTIVE_WORK_AND_NEXT_STEPS.md` as the transfer point.
