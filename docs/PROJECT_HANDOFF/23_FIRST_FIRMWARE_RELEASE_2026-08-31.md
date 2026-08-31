# CoilMaster firmware v1.0.0 — first release checkpoint

Дата: **2026-08-31**

## Release source

Первый production firmware release зафиксирован на exact source SHA:

```text
96aa6c0427090f02f2836526815041495f59ca99
```

Immutable release source branch:

```text
release/coilmaster-v1.0.0
```

`main` не использовался как source.

Основные ветки после добавления release publisher/request и документации продолжают синхронизироваться только non-force. Release binary source при этом намеренно остаётся закреплён на `96aa6c0427090f02f2836526815041495f59ca99`.

## Safe branch synchronization

Перед release ветки расходились на общем base `3003e3ada60e3bc577e4474a079368bd8799905a`:

- production имел один отдельный commit `ed3abc1b2c74ec653a94a711a9523d8dbe3303d0` (`release(web): publish production SD bundle`);
- experiment содержал дальнейшую разработку.

Production-only release metadata было сначала перенесено в experiment, затем создан merge commit с обеими линиями истории:

```text
fbdd4b127a3630b3bd1d002dadc7039bf8112797  retain production SD request
33abe4adc953bdc9979f98460382ddec24fe9546  merge production release metadata history
```

Force update не применялся. После exact release CI production был fast-forwarded на release source, а последующая release/documentation инфраструктура также переносилась non-force.

## Release candidate marker

`platformio.ini` получил только comment-only release checkpoint:

```text
CoilMaster firmware release checkpoint: v1.0.0 / 2026-08-31
```

Build flags, runtime logic и safety semantics этим commit не изменены.

Release candidate/source commit:

```text
96aa6c0427090f02f2836526815041495f59ca99
release: mark CoilMaster firmware v1.0.0 candidate
```

## Exact release CI gate

Все обязательные проверки выполнены на одном exact SHA `96aa6c0427090f02f2836526815041495f59ca99`:

```text
CMP Protocol Tests #4681
run 33361766588
completed / success

ESP32 Build #1831
run 33361766579
completed / success

Arduino RU LCD Build #263
run 33361766582
completed / success
```

Arduino RU LCD workflow дополнительно успешно собрал:

- normal Uno firmware;
- RU/EN Uno firmware;
- ESP32 firmware;
- resource comparison;
- combined release artifact upload.

## Verified Actions build artifact

Combined Actions artifact from exact Arduino RU LCD run `33361766582`:

```text
arduino-ru-lcd-firmware-96aa6c0427090f02f2836526815041495f59ca99
artifact id: 9746946010
size: 12080872 bytes
sha256: dc10d165bad29da9b823e9bb963c1144343784cc12e5f93448d2feae053007ad
expires: 2026-09-30
```

Artifact contains:

```text
Uno normal: firmware.hex + firmware.elf
Uno RU/EN:   firmware.hex + firmware.elf
ESP32:       bootloader.bin + partitions.bin + firmware.bin + firmware.elf
build logs
```

This Actions artifact is the exact verified CI build input used for permanent release packaging.

## Permanent GitHub Release — PUBLISHED

A permanent GitHub Release now exists and is the canonical public release surface:

```text
tag:              v1.0.0
release id:       244615974
name:             CoilMaster Firmware v1.0.0
target commit:    96aa6c0427090f02f2836526815041495f59ca99
draft:            false
prerelease:       false
published_at:     2026-08-31T05:53:21Z
```

Release page:

```text
https://github.com/FantomeKGZ/CoilMaster/releases/tag/v1.0.0
```

Permanent firmware package:

```text
coilmaster-firmware-v1.0.0-96aa6c0427090f02f2836526815041495f59ca99.zip
asset id: 358835596
size: 12047843 bytes
SHA-256: babdb24615772e4613e0dff927292f5662ca2321640c4a684b3cd7afae655421
```

Permanent checksum asset:

```text
coilmaster-firmware-v1.0.0-96aa6c0427090f02f2836526815041495f59ca99.zip.sha256
asset id: 358835597
```

The release ZIP contains:

- Arduino Uno normal firmware (`HEX` + `ELF`);
- Arduino Uno RU/EN LCD firmware (`HEX` + `ELF`);
- ESP32 `bootloader.bin`, `partitions.bin`, `firmware.bin`, `firmware.elf`;
- exact build logs;
- release metadata identifying source SHA and CI runs.

## Permanent release publisher

Repository release infrastructure now includes:

```text
.github/workflows/firmware-release.yml
.github/release/firmware-release-request.json
```

The publisher is fail-closed and verifies before publishing:

- CMP run is completed/success and belongs to the exact requested source SHA;
- ESP32 run is completed/success and belongs to the exact requested source SHA;
- Arduino RU LCD run is completed/success and belongs to the exact requested source SHA;
- all runs belong to this repository and the expected workflow files;
- the exact combined firmware artifact exists and contains all expected binaries;
- the release ZIP receives a fresh SHA-256 checksum;
- an existing release tag is never overwritten.

A later publisher invocation (`Publish Firmware Release` run `33362108344`) ended in failure only at the final duplicate-protection step because `v1.0.0` had already been successfully published. Its earlier validation/download/package steps succeeded. This duplicate-protection failure is **not** a firmware, CI-gate, or first-release failure and must not be interpreted as such.

## Safety invariants retained

Release v1.0.0 does not change the established safety/business invariants:

- physical START remains mandatory;
- no automatic physical START;
- no auto-resume after reboot;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts wire;
- RUN_WIRE remains explicit/manual and exact-provenance-bound;
- append-only RUN/job/history evidence remains authoritative;
- service jobs without repair remain unlinked and do not fabricate repair/spool provenance.

## Release interpretation

For firmware v1.0.0, use these immutable identities:

```text
source SHA: 96aa6c0427090f02f2836526815041495f59ca99
release branch: release/coilmaster-v1.0.0
GitHub tag/release: v1.0.0
release ZIP SHA-256: babdb24615772e4613e0dff927292f5662ca2321640c4a684b3cd7afae655421
```

Later documentation or development commits on `cmp-protocol-v1` / `arduino-ru-lcd-experiment` do not retroactively change the v1.0.0 firmware source or binaries.
