# Release artifact content verification — 2026-08-30

Repository: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: `cmp-protocol-v1`  
Working branch: `arduino-ru-lcd-experiment`

## Purpose

Close the final read-only release-packaging verification after the CI artifact-persistence changes. No firmware, Web, protocol, storage, safety or production branch behavior is changed by this checkpoint.

## Exact CI evidence before this documentation commit

```text
0d8d34f9d9fdbfddaffbc2b6439278980f01629f
CMP Protocol Tests #4637       run 33321899663 / SUCCESS

fa9b16113c60db0a5aaf5d18ec9e925600afbf90
CMP Protocol Tests #4636       run 33321775784 / SUCCESS
Arduino RU LCD Build #242      run 33321775525 / SUCCESS

a38dd2bb53c8af1b795b2cffd7099885791a7d39
CMP Protocol Tests #4635       run 33321756394 / SUCCESS
ESP32 Build #1811              run 33321756392 / SUCCESS
```

## ESP32 release artifact verified by ZIP inspection

Artifact:

```text
artifact id: 9735092955
name: esp32-firmware-a38dd2bb53c8af1b795b2cffd7099885791a7d39
```

Verified files and SHA-256:

```text
bootloader.bin
size: 17536
sha256: 3d234a7471f67b013686dabd4dee7c1fa915c9928463616a94bc9297acf1abf8

partitions.bin
size: 3072
sha256: aaae2888c5a6a348004b5b436f47abb25ae32e72d9003902955a998eda723edd

firmware.bin
size: 1631440
sha256: 6bfc4bcf8dae260479bdb08d14db1e8d2d178943a7a65d0e7df3a4c3137339fa

firmware.elf
size: 26996800
sha256: bd810626c726b8ecc93d1c3a801f5c0fe0a8695c9010e223961a91dc164a5cd7
```

## Arduino RU LCD release artifact verified by ZIP inspection

Artifact:

```text
artifact id: 9735100426
name: arduino-ru-lcd-firmware-fa9b16113c60db0a5aaf5d18ec9e925600afbf90
```

Verified files and SHA-256:

```text
.pio/build/uno/firmware.hex
size: 89232
sha256: 6ade487759620cc4039e22d4207a6ba1bf79f74b0df5f36d68a43ec64744cbe5

.pio/build/uno_ru/firmware.hex
size: 90386
sha256: 9d0e74f26ad511f1e76db4c0ba08a01b4907eba57dd2f8ca99b355b6fe580e1c

.pio/build/uno/firmware.elf
size: 67156
sha256: e748404c48bbda06035996fbefa8c39a92f5d0b3440b20ba89ad30c46da44f11

.pio/build/uno_ru/firmware.elf
size: 68004
sha256: 600ec734ac6557047942ad3f349df6a3e39cbf3719db83a356cb3c55e8b287b3

.pio/build/esp32/bootloader.bin
size: 17536
sha256: 3d234a7471f67b013686dabd4dee7c1fa915c9928463616a94bc9297acf1abf8

.pio/build/esp32/partitions.bin
size: 3072
sha256: aaae2888c5a6a348004b5b436f47abb25ae32e72d9003902955a998eda723edd

.pio/build/esp32/firmware.bin
size: 1631440
sha256: 34c617b335663ba0824cec96137af621bee0bc811f90370aad4031516ab736ff

.pio/build/esp32/firmware.elf
size: 26996800
sha256: 4294073b6b353397b4afa1849944c51fca50a73f2083a4deace9ec0c2069fe6b

uno-build.log
size: 6182
sha256: 7b2eabc52b7baabcd56407924a52105eb99d9c679811f6cbb9390073aae1c382

uno-ru-build.log
size: 5079
sha256: 326f2ea3c6c84cadfa7bdb7a592a18e22052157bfcb3dee107b4326589c1b81c

esp32-build.log
size: 21598
sha256: 9c71e63c8636ba2ddad15bc8ee450bc841fd9a24be8490f48fb9f42c844d219e
```

## ESP32 binary reproducibility note

The ESP32 `firmware.bin` from the two independent GREEN builds is the same size but not byte-for-byte identical.

This is expected and is **not a release blocker**. Direct binary comparison showed only four small differing regions (79 bytes total). The meaningful payload differences contain the intentionally embedded build provenance:

```text
firmware_git_sha:
  a38dd2bb53c8-dirty
  fa9b16113c60-dirty

firmware_build_utc:
  2026-08-30T16:12:12Z
  2026-08-30T16:12:59Z
```

The remaining differing bytes are associated binary metadata/digest/checksum bytes. No firmware/Web runtime source changed between these two packaging commits; `fa9b1611...` only extends the Arduino RU LCD GitHub Actions release-package upload.

Therefore release verification must use the SHA-256 corresponding to the exact artifact/head being flashed rather than expecting binaries from separate commits/build timestamps to match bit-for-bit.

## Production state

At the last exact compare before this checkpoint:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
arduino-ru-lcd-experiment = 0d8d34f9d9fdbfddaffbc2b6439278980f01629f
status: ahead
behind: 0
ahead: 869
merge-base: 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

Production remains untouched. A production fast-forward/merge still requires a separate explicit operator request.

## Current conclusion

The release artifacts have now been verified at three levels:

1. workflow completed successfully;
2. artifact object exists in GitHub Actions with retention metadata;
3. downloaded ZIP contents were inspected and exact file hashes recorded.

No additional repo-side release-packaging blocker was found.

Do not start a new broad speculative audit. Continue only for a concrete reproducible defect, a new explicit requirement, or an explicit production promotion request.

## Safety invariants unchanged

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino remains sole SSR owner;
- ESP32/Web never directly control SSR;
- `RUN_COMPLETED` remains evidence only;
- wire writeoff remains explicit/manual with exact `spool_id + source_session_id + source_run_id`;
- restore/recovery remains operator-controlled, transactional and fail-closed;
- append-only confirmed history is not silently edited/deleted;
- no premature DB/index migration;
- no automatic production-data truncation/rotation/deletion.
