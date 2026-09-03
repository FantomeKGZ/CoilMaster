# CoilMaster — 10-pin 4x4 keypad wiring and Russian variant

Status: verified on a spare Arduino Uno with the physical 10-contact keypad.

## 1. Confirmed keypad matrix

The keypad has 10 physical contacts, but the 4x4 matrix uses only contacts **#2..#9**.

- contacts **#1** and **#10** are not used by the 4x4 matrix and must remain disconnected;
- contacts **#6, #7, #8, #9** are the four matrix rows;
- contacts **#2, #3, #4, #5** are the four matrix columns.

The verified logical layout is:

```text
1 2 3 A
4 5 6 B
7 8 9 C
* 0 # D
```

## 2. Wiring to CoilMaster without firmware pin changes

CoilMaster keeps its existing keypad pin assignment:

- rows: D2, D3, D4, D5;
- columns: D6, D7, D8, D9.

Connect the new 10-contact keypad as follows:

| Keypad contact | CoilMaster / Arduino Uno | Function |
|---:|---:|---|
| #1 | not connected | unused |
| #2 | D6 | COL1 |
| #3 | D7 | COL2 |
| #4 | D8 | COL3 |
| #5 | D9 | COL4 |
| #6 | D2 | ROW1 |
| #7 | D3 | ROW2 |
| #8 | D4 | ROW3 |
| #9 | D5 | ROW4 |
| #10 | not connected | unused |

With this wiring, the existing CoilMaster keypad pin order does **not** need to be changed in firmware.

## 3. Verified standalone test wiring

The keypad was verified on a spare Arduino Uno with this temporary test mapping:

```cpp
byte rowPins[4] = {7, 8, 9, 10}; // keypad contacts #6, #7, #8, #9
byte colPins[4] = {3, 4, 5, 6};  // keypad contacts #2, #3, #4, #5
```

The test correctly produced the expected keys using the standard `Keypad` library.

## 4. Separate Russian-language version

A separate Russian-language firmware/UI variant can be made **without changing the keypad wiring**.

The electrical matrix and the physical key positions remain exactly the same. Only the software interpretation and/or text shown on the LCD changes.

Recommended rule:

- keep the physical matrix and CoilMaster pins identical;
- keep numeric keys `0..9`, `*`, `#` unchanged;
- treat `A`, `B`, `C`, `D` as logical function keys;
- in the Russian build, display Russian names/prompts for those functions instead of changing the electrical key map;
- maintain the Russian version as a separate build/configuration so the standard version remains untouched.

This allows both variants to use the same physical keypad and the same cable/connector wiring.

## 5. Important note

Do not connect keypad contacts #1 and #10 to the CoilMaster keypad GPIOs. They are outside the verified 4x4 matrix and previously caused misleading/fantom scan results when all 10 contacts were treated as independent matrix lines.
