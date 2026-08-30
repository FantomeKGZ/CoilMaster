# Web parity and stale-page sweep — 2026-08-30

Branch: `arduino-ru-lcd-experiment`

## CI baseline

- `CMP Protocol Tests #4596` SUCCESS on `93f0257735480aa4115a3d4335c4bb807a8f91e6`.
- Mobile FTP settings parity commit: `8f46688c99a6a7a22f938947526a7d1cf42fd340`.
- `CMP Protocol Tests #4597` SUCCESS on `8f46688c99a6a7a22f938947526a7d1cf42fd340`.
- Production branch `cmp-protocol-v1` was not modified.

## Closed findings

### FTP settings wording parity

Both desktop and mobile Settings now describe the device FTP server as `FTP / Web recovery` and explicitly state that access is restricted to `/web`.

This matches the real `WebRecoveryFtpServer` behavior. The UI no longer implies general write access to the microSD card.

### Desktop/mobile page parity

The current physical directory listings were compared directly from the branch instead of relying on code-search indexing.

The production feature set exists in both desktop and mobile variants, including:

- clients, motors, repairs and their create/edit/detail flows;
- calculator/conductor conversion;
- warehouse, spools and materials;
- costing, cash and pricing audit;
- winding job/history/reference and Arduino archive;
- backup;
- reports;
- settings, Wi-Fi, RTC/time, Hall and FTP;
- service/recovery job page;
- write-off flow.

The only intentional filename-only difference is `mobile/more.html`, which is the mobile navigation hub.

### Old statistics placeholder

`firmware/esp32/web/desktop/statistics.html` and `firmware/esp32/web/mobile/statistics.html` do not exist.

The former empty/placeholder Statistics page is therefore removed rather than merely hidden.

### Small CRUD pages

The smallest suspicious pages were opened directly and are functional, not placeholders. Examples checked:

- `mobile/client-new.html` performs a real POST to `/api/clients` and routes back to the client flow;
- `desktop/material-new.html` performs a real POST to `/api/materials` with stock/price normalization.

No additional empty-page runtime defect was confirmed in this sweep.

## Existing CI protection

`Tests/Web/check_web_assets.js` walks the current Web tree and checks:

- JavaScript syntax;
- duplicate IDs;
- internal link targets;
- static assets;
- injected UI scripts;
- selected acceptance/safety contracts.

This means removed or renamed internal pages should fail CI when a live HTML link still points to them.

## Remaining low-risk technical debt

`CM_StaticSiteServer.cpp` still contains the older injected desktop navigation icon/version-switch normalizer while `/shared/app-shell.js` is now the authoritative shell owner.

The two paths currently converge safely because the shared shell re-normalizes navigation and removes the older version switch. No user-visible failure was reproduced.

Do not rewrite the large `CM_StaticSiteServer.cpp` solely for this cosmetic cleanup unless a safe, small change can be made and both CMP plus ESP32 build are verified afterwards.

## Next repo-reviewable priorities

1. Continue completeness/performance review only where a concrete runtime or CI gap is demonstrated.
2. Prefer removing stale copy, dead ownership duplication or repeated scans over adding new abstractions/databases.
3. Keep the existing safety invariants unchanged:
   - no automatic physical START;
   - no auto-resume after reboot;
   - ESP32/Web never directly controls SSR;
   - `RUN_COMPLETED` alone never writes off wire;
   - wire write-off stays manual and bound to exact `spool_id`, `source_session_id`, `source_run_id`.
4. Before every existing-file modification, fetch the current branch content and current blob SHA.
5. Never call the current HEAD GREEN unless an exact current run confirms it.
