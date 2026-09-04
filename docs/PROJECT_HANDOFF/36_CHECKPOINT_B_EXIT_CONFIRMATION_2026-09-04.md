# Checkpoint 36 — B exit confirmation during active winding — 2026-09-04

## Goal

Protect the new absolute `B` return-home action against accidental presses while a winding is actively running.

Requested operator behavior:

```text
first B during active winding -> show 5-second confirmation
A -> continue winding
second B -> abandon current winding and return HOME
no key for 5 seconds -> confirmation expires and winding continues
```

## Implementation

Production/source branch: `cmp-protocol-v1`.

Implementation commit:

```text
f7285efecb0f255acacdbb04c1bd4e5f8b48fcdc
feat(arduino): confirm active B exit
```

Changed production Arduino composition root:

```text
firmware/arduino/src/main.cpp
```

Behavior:

- the confirmation guard is armed only when physical keypad `B` is pressed while `MachineState::Winding` is active;
- the running state is not paused merely by opening the confirmation dialog, so the confirmation timeout does not create an automatic restart/START path;
- LCD shows a compact 5 -> 1 countdown with `A=PRODOLZH` and `B=V MENU`;
- `A` dismisses the dialog and leaves the current winding running;
- no key for 5 seconds dismisses the dialog and leaves the current winding running;
- a second `B` confirms exit;
- for a local job, confirmed exit calls the already-hardened local absolute return-home path;
- for an ESP32-owned job, confirmed exit preserves the explicit exact-job-id `OPERATOR_ABORT` path;
- SSR is explicitly forced OFF before the confirmed local/remote abort is applied;
- other keys are consumed while the confirmation dialog is active;
- outside active `Winding`, the established `B` behavior remains unchanged.

This deliberately avoids pausing and then automatically restarting the motor on timeout. Therefore the confirmation feature does not add an automatic physical START/resume mechanism.

## Safety invariants

Unchanged:

- Arduino remains sole SSR owner;
- no Web/ESP32 direct SSR control;
- no automatic physical START;
- no automatic START between repeats;
- no reboot auto-resume;
- `RUN_COMPLETED` still does not write off wire automatically;
- exact remote operator abort correlation remains by exact job id.

## Exact automated verification

For implementation HEAD `f7285efecb0f255acacdbb04c1bd4e5f8b48fcdc`:

```text
CMP Protocol Tests #4899
run 33869409875
completed/success

Arduino Uno Build #259
run 33869409834
completed/success
```

Hardware behavior is not inferred from CI.

## Targeted physical check

1. Enter a local program, for example 260 turns.
2. Start physically and let the count advance.
3. Press `B` once while winding.
4. Confirm the LCD shows the 5-second `A/B` confirmation/countdown and the current winding continues.
5. Press `A`: dialog must disappear and the same winding must continue.
6. Repeat, then press `B` a second time inside the 5-second window: SSR/motor must turn OFF, current program must clear, and HOME must appear.
7. Repeat once more and press nothing: after 5 seconds the dialog must disappear and the winding must continue without creating a new START event.
