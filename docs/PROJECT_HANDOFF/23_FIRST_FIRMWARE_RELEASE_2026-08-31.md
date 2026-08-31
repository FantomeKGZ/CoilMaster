# CoilMaster firmware v1.0.0 — first release checkpoint

Дата: **2026-08-31**

## Release source

Первый firmware release зафиксирован на exact source SHA:

```text
96aa6c0427090f02f2836526815041495f59ca99
```

Release source branch:

```text
release/coilmaster-v1.0.0
```

После release-gate обе основные ветки синхронизированы non-force и указывают на тот же exact SHA:

```text
cmp-protocol-v1          = 96aa6c0427090f02f2836526815041495f59ca99
arduino-ru-lcd-experiment = 96aa6c0427090f02f2836526815041495f59ca99
```

`main` не использовался как source.

## Safe branch synchronization

Перед release ветки расходились на общем base `3003e3ada60e3bc577e4474a079368bd8799905a`:

- production имел один отдельный commit `ed3abc1b2c74ec653a94a711a9523d8dbe3303d0` (`release(web): publish production SD bundle`);
- experiment содержал дальнейшую разработку.

Production-only release metadata было сначала перенесено в experiment, затем создан merge commit с обеими линиями истории:

```text
fbdd4b127a3630b3bd1d002dadc7039bf8112797  retain production SD request
33abe4adc953bdc9979f98460382ddec24fe9546  merge production release metadata history
```

Force update не применялся. После exact release CI production был fast-forwarded на release source SHA.

## Release candidate marker

`platformio.ini` получил только comment-only release checkpoint:

```text
CoilMaster firmware release checkpoint: v1.0.0 / 2026-08-31
```

Build flags, runtime logic и safety semantics этим commit не изменены.

Release candidate commit:

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

## Firmware artifact

Combined Actions artifact:

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

This Actions artifact is the verified binary package for the first firmware release source SHA.

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

## GitHub Release object status

The current connected GitHub tooling can create/update branches and repository files and read/download Actions artifacts, but does not expose creation of a GitHub Release/tag object. Therefore this checkpoint intentionally does **not** claim that a GitHub Release object/tag `v1.0.0` exists.

The immutable source pointer for this release is the branch:

```text
release/coilmaster-v1.0.0
```

and the exact verified binary package is Actions artifact `9746946010` from run `33361766582`.
