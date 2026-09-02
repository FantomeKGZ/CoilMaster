# Physical Arduino + ESP32 E2E acceptance — 2026-08-31

Branch under test: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — do not modify.

## Purpose

This is the next mandatory engineering gate after checkpoint 168. Do not replace it with another speculative software optimization.

The goal is to validate the real two-board CoilMaster path end-to-end while preserving all safety and provenance invariants.

## CI status of this acceptance checkpoint

The documentation/checklist baseline itself is exact-CMP-confirmed:

```text
8b888381124264cd8199fb1d2124fe04772fedcc
CMP Protocol Tests #4753  run 33373426510 / SUCCESS

ba5f2406ab2da544b8b3520c93ea1f7d48b7e42c
CMP Protocol Tests #4754  run 33373490375 / SUCCESS

fb89cc9a906393d6a2c45c5a57a0323b1f022e37
CMP Protocol Tests #4755  run 33373645651 / SUCCESS

401812bfc0104ea77a0902cccaf6f8c4b283b066
CMP Protocol Tests #4757  run 33410394016 / SUCCESS
```

`401812bf...` is exact-CMP-confirmed for the documentation commit that records the first observed physical cold-boot PASS. Any later documentation-only update is newer and must receive its own exact SUCCESS before that newer HEAD itself is called GREEN.

## Before power-on

Record the exact firmware source SHA flashed to both boards.

Preferred current software baseline before this checklist was created:

```text
75b36c9875fd1fd271b433e8db4b940d43835fea
CMP #4748       run 33372615230 / SUCCESS
ESP32 #1850     run 33372615249 / SUCCESS
Arduino RU #281 run 33372615235 / SUCCESS
```

Later documentation/test-only commits do not automatically replace this firmware evidence.

Verify hardware state:

- ESP32 and Arduino Uno connected through the normal project UART path;
- LCD and keypad connected;
- Hall sensor connected as used by the project;
- SSR/motor power path safe for a controlled test;
- emergency/manual stop available;
- microSD present if the production data path requires it;
- browser can reach the live ESP32 UI.

## E2E sequence

### 1. Boot / fail-closed state

Power both boards from a normal cold boot.

Pass conditions:

- Arduino does not start the motor automatically;
- ESP32 does not directly switch SSR;
- no old job resumes automatically;
- keypad responds before Hall mode;
- normal RU LCD screens are readable;
- web UI loads and reports the connected system without initiating motion.

Record:

```text
BOOT PASS/FAIL:
LCD text:
Keypad result:
Unexpected motor/SSR activity: YES/NO
```

#### Observed physical result — 2026-08-31

User-confirmed cold-boot evidence:

```text
BOOT: PASS for confirmed items
Keypad result: PASS / works
Unexpected motor/SSR activity: NO
Web ESP32: PASS / works
LCD text: not recorded yet
```

This confirms the three reported physical safety/usability checks. It does **not** mark the full E2E acceptance complete; exact LCD text and all later sections remain pending.

### 2. ESP32 job transfer to Arduino

From the normal CoilMaster Web flow, prepare one controlled winding job with known motor/repair context and an exact selected spool.

Pass conditions:

- ESP32 sends the command/session to Arduino;
- Arduino acknowledges the command;
- job becomes visible/prepared on Arduino;
- motor still does not start;
- no wire is written off yet.

Record exact identifiers shown by the system:

```text
motor_id:
repair_id:
job/session_id:
run_id before physical start, if allocated:
spool_id:
command/ack result:
```

#### Physical UI defect found — 2026-09-02

During the real Arduino-side acceptance check, the operator confirmed that cancelling an ESP32 remote job actually requires physical key `B`, while the READY-screen hint still exposed `C` as the secondary action. Source inspection confirmed that the safety behavior is correct: `B` is `ReturnHome` / explicit remote-job operator abort and `C` is manual mode.

Fix committed on development branch only:

```text
f20625ce05b4e34f3bbf5fd8f4630fcb130d165c
fix(arduino): show B cancel for remote ready job
```

For `JobSource::Esp32Web` in READY state the LCD now shows:

```text
RU: A=СТАРТ B=ОТМЕНА
EN: A=START B=CANCEL
```

For local keypad jobs the existing `C=manual` hint remains unchanged. No SSR/start ownership or remote-cancel behavior was changed. This commit is **not** called GREEN until exact CI success is verified.

### 3. Physical START ownership

Start the prepared winding only with the physical Arduino-side START action.

Pass conditions:

- browser/ESP32 alone cannot start the motor;
- physical START causes the run to begin;
- exactly one RUN_STARTED evidence row/event is produced for the exact session/run;
- no duplicate/repeat start occurs automatically.

Record:

```text
physical START accepted: YES/NO
RUN_STARTED observed: YES/NO
source_session_id:
source_run_id:
SSR/motor started only after physical START: YES/NO
```

### 4. Physical completion

Allow the controlled run to complete normally.

Pass conditions:

- motor stops correctly;
- exactly one RUN_COMPLETED evidence row/event is produced for the same session/run;
- RUN_COMPLETED does not deduct wire automatically;
- completed job appears in the expected Arduino/Web archive/status view.

Record:

```text
RUN_COMPLETED observed: YES/NO
same source_session_id: YES/NO
same source_run_id: YES/NO
wire stock changed automatically: YES/NO (must be NO)
completed task visible: YES/NO
```

### 5. Manual exact RUN_WIRE writeoff

From the normal operator Warehouse/repair flow, perform the manual wire writeoff for the completed run.

Mandatory exact provenance:

```text
spool_id
source_session_id
source_run_id
```

Pass conditions:

- writeoff requires explicit operator action;
- exact spool is used;
- exact completed session/run is referenced;
- duplicate writeoff/replay is rejected or idempotently resolved according to the existing contract;
- costing reflects the persisted wire usage after the successful writeoff.

Record:

```text
spool_id:
source_session_id:
source_run_id:
quantity/weight written off:
writeoff result:
stock before:
stock after:
costing updated: YES/NO
```

### 6. Repair costing / finalization evidence

Verify the same repair after the run/writeoff.

Pass conditions:

- material/wire cost appears in repair costing;
- run evidence remains linked/read-only;
- finalization preflight sees the required winding/writeoff evidence;
- closing/finalizing the repair does not rewrite append-only history;
- report/archive views show the completed evidence chain.

Record:

```text
repair costing correct: YES/NO
finalization preflight: PASS/FAIL
report/archive evidence visible: YES/NO
repair close tested: YES/NO
```

### 7. Hall RU LCD acceptance

With the machine in a safe idle state, test Hall mode.

Expected reachable LCD states:

```text
ДАТЧИК ХОЛЛА
A ИЛИ START

ТЕСТ ХОЛЛА
ОСТ. <n> СЕК

СОХР. НАСТР.?
#=ДА B=НЕТ
```

Pass conditions:

- keypad responds before Hall mode;
- Hall test starts only after local Arduino confirmation;
- 15-second Hall flow completes;
- both apply and reject paths are usable;
- normal RU LCD/CGRAM rendering is restored after Hall exit;
- keypad still responds after Hall mode;
- Hall mode does not create a motor START path through ESP32/Web.

Record:

```text
Hall arm screen: PASS/FAIL
15-second run: PASS/FAIL
apply path: PASS/FAIL
reject path: PASS/FAIL
normal LCD restored: PASS/FAIL
keypad after Hall: PASS/FAIL
```

### 8. Reboot/recovery fail-closed check

With no dangerous active motion, prepare a job and reboot/power-cycle before physical START. Separately verify post-completion idle reboot behavior.

Pass conditions:

- reboot never automatically starts the motor;
- no automatic repeat START;
- no unsafe automatic resume;
- recovery state is explicit/fail-closed;
- historical RUN_STARTED/RUN_COMPLETED and writeoff evidence remain immutable.

Record:

```text
reboot before START -> automatic motion: YES/NO (must be NO)
reboot caused auto-resume: YES/NO (must be NO)
recovery/manual-review state:
existing run/writeoff evidence preserved: YES/NO
```

## Mandatory safety acceptance

All must be true:

- [ ] no automatic physical START;
- [ ] no automatic repeat START;
- [ ] no auto-resume after reboot;
- [ ] Arduino remains sole SSR owner;
- [ ] ESP32/Web never directly controls SSR;
- [ ] RUN_COMPLETED alone does not deduct wire;
- [ ] wire writeoff is explicit/manual;
- [ ] writeoff uses exact `spool_id + source_session_id + source_run_id`;
- [ ] append-only evidence remains immutable;
- [ ] fail-closed recovery remains intact.

## Evidence to send back to the development chat

Send the observed values/results from the sections above. The minimum useful evidence is:

```text
1. Exact flashed SHA(s)
2. ESP32 -> Arduino command/ACK result
3. source_session_id
4. source_run_id
5. spool_id
6. RUN_STARTED observed
7. RUN_COMPLETED observed
8. confirmation that wire did NOT auto-deduct
9. manual RUN_WIRE result + stock before/after
10. costing/finalization result
11. keypad before/after Hall
12. Hall apply/reject result
13. reboot/no-auto-resume result
```

If any item fails, stop the acceptance run at that point and report the exact visible/logged symptom. Fix only the confirmed defect before resuming the checklist.
