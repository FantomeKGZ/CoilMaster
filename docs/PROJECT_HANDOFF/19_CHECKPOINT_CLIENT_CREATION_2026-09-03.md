# Checkpoint 19 — client creation form

Date: 2026-09-03
Branch: `arduino-ru-lcd-experiment`
Production `cmp-protocol-v1` not changed.

## Scope

Desktop/mobile client creation was reworked without changing the canonical client storage model.

Authoritative required fields remain exactly the existing backend contract:

- `name` — required;
- `phone` — required and must normalize to at least 7 digits.

Optional field remains:

- `comment` — optional.

No new mandatory client fields were introduced. Existing records and `/api/clients` storage semantics are unchanged.

## UI behavior

Both `desktop/client-new.html` and `mobile/client-new.html` now:

- explicitly separate `Обязательные данные` and `Дополнительно`;
- mark only name and phone as required;
- explain that comment can be added later;
- trim submitted values;
- validate phone for at least 7 digits before POST while backend validation remains authoritative;
- retain single-flight submit protection;
- POST to canonical `/api/clients` only;
- do not create a repair or motor relationship automatically.

Desktop preserves the existing canonical repair handoff through `/desktop/repairs.html?client_id=...`.
Mobile opens the newly created client card after successful creation.

## Regression coverage

Added `Tests/Web/check_client_new_ui.js` and wired it through `Tests/Web/check_web_assets.js`.
The contract locks:

- name required;
- phone required;
- comment optional;
- field bounds 96 / 48 / 320;
- client-side 7-digit validation;
- canonical POST endpoint;
- backend name/phone required and normalized-phone minimum.

## CI history

Intermediate run:

`CMP Protocol Tests #4780`, run `33717926838`, head `ff824dbc473ed812a4bd39283861b5216a0b3921` — FAILURE.

The new client regression itself passed under `Audit web JavaScript and navigation`. Failure was a pre-existing CRM contract expectation for the desktop post-create repair handoff after it was unnecessarily changed directly to `repair-new.html`. The canonical `/desktop/repairs.html?client_id=...` handoff was restored in commit `7c63d6838f715471751634f66ce296ea7c4b9228`.

Do not call the current documentation HEAD GREEN until its exact CI run is `completed/success`.
