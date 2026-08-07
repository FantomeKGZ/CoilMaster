# Motor Card ↔ Handbook Scheme Binding Specification

Status: `PROPOSED`

Priority: `P1`

Date: 2026-08-07

## 1. Purpose

Define how a CoilMaster motor record on ESP32 references a winding-layout scheme and one selected connection scheme from the external/handbook scheme catalog without duplicating the original handbook images or HTML inside the motor database.

This specification is intentionally data-first. UI implementation must follow the model instead of embedding handbook file paths ad hoc into motor cards.

## 2. Core architecture decision

A motor record does **not own** a winding diagram.

A motor record stores a **binding** to a stable handbook scheme identifier.

The handbook catalog owns:

- winding-layout metadata;
- the winding-layout image;
- all related connection pages;
- all connection images;
- supported connection types;
- legacy source paths.

The motor record owns only:

- which winding-layout scheme was selected for this motor;
- optionally which connection variant was actually used;
- binding metadata needed for validation/history.

Required relationship:

`Motor record -> scheme_id -> handbook catalog -> layout + all connection options`

## 3. Why identifiers must be used

Legacy paths such as `y363000.html` and `ss2k3000a2.html` are implementation details of the old handbook.

The persistent motor database must prefer stable CoilMaster identifiers:

- `CM-SCH-*` for a winding-layout variant;
- `CM-CON-*` for a concrete connection option (planned catalog extension).

Legacy file paths may be stored only as fallback/debug metadata, never as the primary permanent foreign key.

## 4. Motor record extension

Recommended optional block:

```json
{
  "winding_reference": {
    "version": 1,
    "scheme_id": "CM-SCH-XXXXXXXXXXXX",
    "connection_id": "CM-CON-XXXXXXXXXXXX",
    "catalog_version": 2,
    "bound_at": "2026-08-07T10:00:00Z",
    "binding_source": "user",
    "verification": "confirmed"
  }
}
```

`winding_reference` is optional so existing motor records remain valid.

### Field rules

- `version`: version of this binding object.
- `scheme_id`: selected winding-layout scheme. Required when the block exists.
- `connection_id`: optional selected connection option actually used for this motor.
- `catalog_version`: catalog format/version used at binding time.
- `bound_at`: optional audit timestamp.
- `binding_source`: `user`, `suggested`, `imported`, or future approved value.
- `verification`: `unverified`, `suggested`, or `confirmed`.

The database must not copy the complete `connections[]` array from the handbook catalog into every motor record.

## 5. Distinguish available vs selected connection schemes

This distinction is mandatory.

### Available connection options

Come from the handbook catalog for the selected `scheme_id`.

A winding-layout scheme may have:

- zero connection pages;
- one connection page;
- several connection pages.

The catalog already demonstrates that multiple connection pages are common, therefore the UI must never assume a one-to-one layout-to-connection relationship.

### Selected connection option

The motor record may store one `connection_id` representing what was actually used for this motor.

If no connection was explicitly selected, `connection_id` remains absent/null even if the catalog exposes multiple valid options.

## 6. Motor card UI

The motor card should remain compact.

When `winding_reference.scheme_id` is present, add a block:

`Обмотка и схемы`

Recommended compact layout:

- small winding-layout preview;
- short metadata line (`slots / rpm / 2p / q / y / a` when known);
- `Открыть схему` action;
- `Схем подключения: N` indicator;
- one or two compact connection previews;
- `Все схемы подключения (N)` when more than two exist;
- `Изменить привязку` action in edit mode.

If one connection is selected for this motor, mark it clearly as:

`Использовано на этом двигателе`.

Do not label every catalog connection as used.

## 7. Add/edit motor flow

Creation and editing must use the same reusable `Scheme Picker` component.

### Suggested flow

1. User enters normal motor/winding parameters.
2. UI requests candidates from the handbook catalog using known fields such as:
   - slots;
   - rpm;
   - poles/2p;
   - q;
   - winding type;
   - parallel branches;
   - winding pitch;
   - connection/special attributes when available.
3. UI shows candidate winding-layout previews.
4. User selects a winding-layout scheme.
5. UI shows **all** connection options belonging to that scheme.
6. User may optionally choose the connection actually used.
7. UI saves only stable identifiers to the motor record.

The UI must retain an `Открыть в справочнике` action for full manual verification.

## 8. Preview strategy

Do not duplicate full legacy images into the motor database.

Recommended model:

- full original images remain in the handbook/archive;
- generated thumbnails/previews are stored in the handbook/static data layer;
- motor cards load small previews by `scheme_id` / `connection_id`;
- clicking a preview opens the original high-resolution handbook material.

Suggested future derived assets:

```text
handbook/shared/previews/schemes/<scheme_id>.webp
handbook/shared/previews/connections/<connection_id>.webp
```

Preview generation must never modify original images.

## 9. Offline and failure behaviour

The motor database must not depend on the handbook being reachable.

Required states:

### Full handbook available

Show previews, metadata and navigation.

### Catalog available but image unavailable

Show identifiers and metadata, and an explicit `Предпросмотр недоступен` state.

### Handbook/catalog unavailable

Motor card remains usable. Show the stored `scheme_id` / selected connection state and mark the handbook reference as temporarily unavailable.

A handbook/network failure must never prevent opening or editing the core motor record.

## 10. Data ownership and load on ESP32

ESP32 remains responsible for the motor database and the CoilMaster web/API layer.

The scheme catalog is read-oriented reference data. The ESP32 must not re-parse hundreds of legacy HTML pages for every motor-card request.

Preferred future modes:

1. External handbook site hosts catalog/previews; ESP32 stores only IDs.
2. Optional synchronized compact catalog subset on microSD for offline use.

In both modes, a motor card read should require only small JSON metadata and small preview assets, not full archive traversal.

## 11. REST API proposal

Exact routes remain subject to the main REST API design, but the data contract should support:

```text
GET  /api/motors/:id
POST /api/motors
PUT  /api/motors/:id
```

where the motor body optionally contains `winding_reference`.

For handbook integration, one of these patterns may be used:

```text
GET /api/handbook/schemes?slots=36&rpm=3000
GET /api/handbook/schemes/:scheme_id
GET /api/handbook/connections/:connection_id
```

or the browser may query the external handbook directly when deployment/security allows it.

No endpoint in this feature may directly control winding hardware.

## 12. Validation rules

When saving a motor binding:

- `scheme_id` must exist in the active catalog when catalog validation is available;
- `connection_id`, if present, must belong to the selected `scheme_id`;
- the UI must warn if motor parameters conflict with the selected scheme;
- a conflict may require explicit user confirmation rather than silent replacement;
- removing a binding must never delete handbook files;
- deleting a motor must never delete handbook schemes.

## 13. Catalog evolution

The handbook currently has stable `CM-SCH-*` identifiers for layout variants.

Before this motor-card feature is implemented fully, the catalog should be extended so each concrete connection choice can also receive a stable deterministic `CM-CON-*` identifier.

Recommended identity basis:

`connection page + concrete connection image + normalized type`

If a page has only a page-level connection and no reliably separable image, a page-level connection identifier may be generated, but the distinction must be represented in metadata.

## 14. History and future compatibility

The data model should leave room for recording differences between:

- handbook reference scheme;
- actual repair/winding performed.

Do not add the full history model in the first implementation, but avoid a schema that makes it impossible later.

Possible future block:

```json
{
  "actual_winding": {
    "pitch": "9",
    "parallel_branches": 2,
    "notes": "..."
  }
}
```

This must remain separate from the reference catalog entity.

## 15. Implementation phases

### Phase A — Catalog contract

- add deterministic `CM-CON-*` IDs;
- validate connection ownership under each `CM-SCH-*`;
- generate preview metadata;
- document catalog version upgrade.

### Phase B — ESP32 data model

- add optional `winding_reference` to motor record schema;
- preserve backward compatibility with existing records;
- add validation and migration tests.

### Phase C — Read-only motor-card widget

- display selected scheme preview;
- display connection count and selected connection;
- full-handbook navigation;
- graceful offline state.

### Phase D — Scheme Picker

- use one component for add/edit motor;
- candidate filtering by motor parameters;
- explicit user selection;
- show all connection options;
- store selected IDs only.

### Phase E — Offline optimization

- optional compact catalog cache on microSD;
- preview cache policy;
- catalog version/update policy.

## 16. Acceptance criteria

Feature is complete only when:

- existing motors without bindings still work;
- a motor can bind to one `CM-SCH-*`;
- one scheme can expose zero, one, or many connection options;
- the UI clearly reports the number of connection options;
- the user can select one connection actually used for the motor;
- motor storage does not duplicate full handbook images or all connection metadata;
- unavailable handbook data does not break the motor card;
- add and edit use the same picker;
- no hardware-control path is introduced;
- documentation and tests are updated.

## 17. Decision summary

Recommended architecture:

`Motor database owns references; handbook catalog owns schemes.`

This keeps ESP32 storage small, preserves the legacy handbook as source material, supports multiple connection schemes per winding layout, and provides a clean path to motor-card previews, editing, history and future synchronization.
