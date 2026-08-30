# Service-job desktop/mobile parity — 2026-08-30

Branch: `arduino-ru-lcd-experiment`

## Finding

The desktop and mobile `service-job.html` pages used the same authoritative backend routes and essentially the same safety eligibility checks, but the operator UX diverged after a mutation was submitted.

Mobile immediately disabled the relevant review/cancel/dismiss button while its POST was pending. Desktop left the button active until the request returned or the next status refresh, allowing an operator double click to issue a duplicate POST.

The backend remained authoritative and idempotent/fail-closed where required, so this was not a bypass of machine safety. It was still a real desktop/mobile parity and duplicate-submit defect.

## Fix

Commit:

```text
ecab08290678381420d15667fc3d82b823498a66
```

Desktop `firmware/esp32/web/desktop/service-job.html` now:

- disables manual-review confirmation immediately after explicit operator confirmation;
- disables remote-cancel immediately while the request/Arduino acknowledgement is pending;
- disables dismiss immediately while the request is pending;
- restores the relevant button only if the HTTP request fails;
- shows the pending Arduino acknowledgement state on the cancel button;
- includes the same explicit `WAITING_ARDUINO_ACK` and linked-repair state explanations already present in mobile.

No API, persisted-state, UART, START, SSR, recovery or cancellation eligibility rule changed.

## Exact CI evidence

```text
CMP Protocol Tests #4599
run 33318941365
head ecab08290678381420d15667fc3d82b823498a66
completed / success
```

ESP32 Build #1792 and Arduino RU LCD #221 were still in progress at the first exact check, so they must not be cited as GREEN unless later metadata confirms completion/success.

## Safety invariants unchanged

- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- unresolved delivery/cancel states remain fail-closed;
- linked repair jobs cannot be dismissed through the service-job UI;
- remote cancel remains limited to an unlinked accepted job with zero physical run evidence;
- `RUN_COMPLETED` still does not perform automatic wire write-off.

## Next

Continue only evidence-based completeness/performance review. Prefer a concrete operator-visible defect, stale ownership, unsafe duplicate action, or demonstrated repeated scan over speculative refactoring.
