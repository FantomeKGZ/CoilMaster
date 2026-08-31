# Checkpoint — mobile motor parity repair — 2026-08-31

Repo: `FantomeKGZ/CoilMaster`  
Working branch: `arduino-ru-lcd-experiment`  
Production `cmp-protocol-v1` unchanged.

## Trigger

Operator reported that recently added desktop/Web motor changes were not visible in the mobile UI.

## Proven gaps

The audit confirmed two stale mobile surfaces:

1. `firmware/esp32/web/mobile/motor-details.html` still used only legacy `coil_program` and did not expose the newer versioned WORKING/STARTING workflow.
2. `firmware/esp32/web/mobile/motors.html` still rendered the legacy motor program rather than authoritative latest WORKING/STARTING roles.

The following recently completed mobile surfaces were already aligned and did not require another implementation change:

- `mobile/motor-edit.html` already supports canonical WORKING/STARTING editing via `/api/motors/winding/role` with `expected_winding_version_id`.
- mobile dashboard already shares `completed-job-display-reset.js`, so last-sent / last-completed display is common with desktop.
- mobile Arduino archive already shares `arduino-windings-archive.js` and has the compact task-list renderer.

## Fix

### Mobile motor card

Commit:

```text
0b23d69ca7aabbf4fe21d17e3e20fa02f1d8f259
fix(web): restore mobile motor details parity
```

Mobile motor details now includes:

- authoritative latest versioned WORKING and STARTING cards;
- legacy WORKING fallback only when no winding version exists;
- bounded winding-version history (`limit=12`);
- direct shared `motor-role-send.js` service-send action under both role cards;
- no repair required for that direct service send;
- physical START remains mandatory and Web never controls SSR;
- repair-linked WORKING/STARTING navigation remains available separately;
- AS_RECEIVED vs after-repair comparison;
- bounded repair-version lookup (`limit=24`);
- bounded motor repair history.

### Mobile motor catalogue

Commit:

```text
819699251eb51e87d9b2bf47872e45d63b7ac5ca
fix(web): align mobile motor catalogue roles
```

The mobile catalogue now rereads `/api/motors/winding/latest?motor_id=...` for the bounded visible page and displays WORKING and STARTING separately. Legacy data is explicitly labelled when a versioned winding has not yet been created.

### Regression protection

Commits:

```text
33783219386ac97c169ab59d02e82807badbc994
test(web): guard mobile motor parity

3b9688b7525477b75b58353bfc119f5ca13984d0
test(web): accept equivalent bounded quote styles
```

The test now requires desktop/mobile parity for:

- versioned WORKING/STARTING motor detail cards;
- direct shared service-send helper;
- winding version history;
- AS_RECEIVED comparison;
- repair-linked role navigation;
- motor edit workflow;
- versioned role display in both motor catalogues;
- physical START / SSR safety wording.

## CI note

`CMP #4676` on `337832193...` failed only in the newly expanded source-text parity assertion because desktop used `limit:"24"` while mobile used `limit:'24'`. Runtime CTest and all unrelated audits passed; the assertion was corrected without changing runtime behavior.

Replacement exact test HEAD:

```text
3b9688b7525477b75b58353bfc119f5ca13984d0
CMP #4677 / run 33361048029
```

At documentation creation time the run was still in progress, but the targeted `Audit motor details and repair history contracts` step had already completed SUCCESS. Do not call the whole run GREEN until GitHub reports `completed/success`.

## Deployment scope

This parity repair is Web-only. Updating the deployed `/web` tree is sufficient; no Arduino or ESP32 firmware change was introduced by this checkpoint.
